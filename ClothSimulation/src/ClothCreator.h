#ifndef CLOTHCREATOR_H
#define CLOTHCREATOR_H

#include <map>
#include <CDT/CDT.h>
#include <dxf/dl_dxf.h>
#include <dxf/dl_creationadapter.h>
#include "test_creationclass.h"

#include "Cloth.h"

// Defaults
const float STEP = 50.0f;
const glm::vec3 CLOTH_POSITION = glm::vec3(-3.0f, 9.0f, 0.0f);

class ClothCreator
{
public:
    glm::vec3 clothPos = CLOTH_POSITION;     // world position of cloth
    std::vector<Cloth*> cloths;  // cloths parsed from .dxf file;
    const float step = STEP;     // steps between points

    // dxf parser
    Test_CreationClass* creationClass;
    DL_Dxf* dxf;

    ClothCreator(const std::string& clothFilePath) {
        createCloths(clothFilePath);
    }

    ~ClothCreator() {
        delete dxf;
        delete creationClass;

        for (Cloth* c : cloths) {
            delete c;
        }
    }

private:
    /*
     * dxf file parser
     */
    std::vector<std::vector<point2D>>*
        readClothData(const std::string& clothFilePath) {
        std::cout << "Reading file " << clothFilePath << "...\n";

        creationClass = new Test_CreationClass();
        dxf = new DL_Dxf();
        if (!dxf->in(clothFilePath, creationClass)) { // if file open failed
            std::cerr << clothFilePath << " could not be opened.\n";
            return nullptr;
        }

        return &(creationClass->blockNodes);
    }

    /*
     * create cloths using VERTEX data
     */
    void createCloths(const std::string& clothFilePath) {

        std::vector<std::vector<point2D>>* clothNodes = readClothData(clothFilePath);
        assert(clothNodes != nullptr);

        // use for loop to retrieve all cloths
        for (size_t i = 0, sz = clothNodes->size(); i < sz; i++) {

            // boundary of bounding box
            float minX = FLT_MAX, minY = FLT_MAX;
            float maxX = -FLT_MAX, maxY = -FLT_MAX;

            // Constrained Delaunay Triangulation(CDT)
            // ---------------------------------------
            CDT::Triangulation<float> cdt;
            std::vector<CDT::V2d<float>> contour;
            std::vector<CDT::Edge> edges;

            // add vertex into contour; contour should be closed
            for (size_t j = 0, ctr_sz = (*clothNodes)[i].size(); j < ctr_sz; j++) {
                const point2D& p = (*clothNodes)[i][j];
                contour.push_back({ p.first, p.second });

                updateBoundary(p, minX, maxX, minY, maxY);

                if (j == ctr_sz - 1) {
                    edges.push_back({ CDT::VertInd(0), CDT::VertInd(j) });
                }
                else {
                    edges.push_back({ CDT::VertInd(j), CDT::VertInd(j + 1) });
                }
            }

            std::cout << "contour size: " << contour.size() << "\n";

            // randomly generate vertex in the bounding box
            // if the vertex lies outside of contour, eraseOuterTrianglesAndHoles will erase it
            // add random offset to x, y to mimic real cloth
            int cnt = 0;
            for (float x = minX; x < maxX; x += step) {
                for (float y = minY; y < maxY; y += step) {
                    // you can modify params directly
                    contour.push_back({ x + (cnt % 7) * 0.7f, y - (cnt % 7) * 1.0f });
                    cnt += 1;
                }
            }

            CDT::RemoveDuplicatesAndRemapEdges(contour, edges);
            cdt.insertVertices(contour);
            cdt.insertEdges(edges);
            cdt.eraseOuterTrianglesAndHoles();

            Cloth* cloth = new Cloth(clothPos, minX, maxX, minY, maxY);
            createCloth(cdt, cloth);
            cloths.push_back(cloth);

            // TODO: delete me
            // for debug purpose
            if (i == 1) break;
        }
    }

    /*
     * create A cloth
     */
    void createCloth(CDT::Triangulation<float>& cdt, Cloth* cloth) {

        // store index of nodes lying on the contour
        std::map<CDT::VertInd, int> contourIndex;
        for (const CDT::Edge& e : cdt.fixedEdges) {
            ++contourIndex[e.v1()];
            ++contourIndex[e.v2()];
        }

        // generate Node of Cloth
        for (size_t j = 0; j < cdt.vertices.size(); j++) {
            const CDT::V2d<float>& p = cdt.vertices[j];
            Node* n = new Node(p.x, p.y, 0.0f);
            n->lastWorldPosition = n->worldPosition = cloth->modelMatrix * glm::vec4(n->localPosition, 1.0f);

            cloth->nodes.push_back(n);

            if (contourIndex.count(j)) {
                cloth->contour.push_back(n);
            }
        }

        // Find Turning Nodes
        // ------------------
        // for every node 'middle' in contour, find its neighbors 'prev' and 'next'
        // we can get 2 vectors: v1 = middle - prev, v2 = next - middle
        // if theta between (n1, n2) > threshold, then we take 'middle' as a 'turning point'
        Node* prev, * middle, * next;
        int index;  // index of one of the turning point
        for (int j = 0, ctr_sz = cloth->contour.size(); j < ctr_sz; j++) {
            middle = cloth->contour[j];
            prev = cloth->contour[((j - 1) + ctr_sz) % ctr_sz];
            next = cloth->contour[(j + 1) % ctr_sz];

            glm::vec3 v1 = middle->worldPosition - prev->worldPosition;
            glm::vec3 v2 = next->worldPosition - middle->worldPosition;
            float len1 = glm::length(v1);
            float len2 = glm::length(v2);
            assert(len1 * len2 > 1e-5);  // avoid dividing by zero

            float cos = glm::dot(v1, v2) / (len1 * len2);
            middle->isTurningPoint = glm::dot(v1, v2) < 0 || (cos < glm::cos(20));
            if (middle->isTurningPoint) {
                index = j;
            }
        }

        // Generate Contour Segments
        // -------------------------
        // divide contour into several segments
        // each segments have plenty of points; points in the same segment have same segmentID
        // ©°---*---*----©´
        // ©¸----*----*--©¼
        std::vector<Node*> segment;
        int segmentID = 0;
        for (int cnt = 0, ctr_sz = cloth->contour.size(); cnt <= ctr_sz; ++cnt) {
            Node* n = cloth->contour[(index + cnt) % ctr_sz];
            if (n->isTurningPoint && cnt != 0) {
                cloth->segments.push_back(segment);
                std::cout << "segment size: " << segment.size() << "\n";
                segment.clear();
                segmentID += 1;
            }
            n->segmentID = segmentID;
            segment.push_back(n);
        }


        std::cout << "# tri:" << cdt.triangles.size() << std::endl;
        // retrieve triangles, generate springs and faces
        Node* n1, * n2, * n3;
        for (const CDT::Triangle& tri : cdt.triangles) {
            CDT::VerticesArr3 triIndex = tri.vertices;
            n1 = cloth->nodes[triIndex[0]];
            n2 = cloth->nodes[triIndex[1]];
            n3 = cloth->nodes[triIndex[2]];
            cloth->faces.push_back(n1);
            cloth->faces.push_back(n2);
            cloth->faces.push_back(n3);
            // TODO: add different forces
            cloth->springs.push_back(new Spring(n1, n2, cloth->structuralCoef));
            cloth->springs.push_back(new Spring(n1, n3, cloth->structuralCoef));
            cloth->springs.push_back(new Spring(n2, n3, cloth->structuralCoef));

            // traverse 3 neightbours, add springs between diagonal points
            for (size_t j = 0; j < 3; j++) {
                if (tri.neighbors[j] >= cdt.triangles.size()) continue;
                CDT::VerticesArr3 neighborTriIndex = (cdt.triangles[tri.neighbors[j]]).vertices;

                // in triIndex and neightborTriIndex, two indices are identical
                // we need to find the third index and connect them
                //     ind2
                //     /|\
                //    / | \
                //   /__|__\
                //   \  |  /
                //    \ | /
                //     \|/
                //     ind1
                CDT::VertInd ind1, ind2;
                for (size_t k = 0; k < 3; k++) {
                    ind1 = triIndex[k];
                    if (ind1 != neighborTriIndex[0] && ind1 != neighborTriIndex[1] && ind1 != neighborTriIndex[2]) {
                        break;
                    }
                }
                for (size_t k = 0; k < 3; k++) {
                    ind2 = neighborTriIndex[k];
                    if (ind2 != triIndex[0] && ind2 != triIndex[1] && ind2 != triIndex[2]) {
                        break;
                    }
                }
                cloth->springs.push_back(new Spring(cloth->nodes[ind1], cloth->nodes[ind2], cloth->bendingCoef));
            }
        }
        std::cout << "Initialize cloth with " << cloth->nodes.size() << " nodes and " << cloth->faces.size() / 3 << " triangles\n";
    }

    void updateBoundary(point2D point, float& minX, float& maxX, float& minY, float& maxY) {
        float x = point.first;
        float y = point.second;

        if (x < minX) minX = x;
        if (x > maxX) maxX = x;

        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
    }
};

#endif