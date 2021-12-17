#ifndef CLOTHCREATOR_H
#define CLOTHCREATOR_H

#include <map>
#include <CDT/CDT.h>
#include <dxf/dl_dxf.h>
#include <dxf/dl_creationadapter.h>
#include "test_creationclass.h"

#include "Cloth.h"

// Defaults
const float STEP = 25.0f;
const glm::vec3 CLOTH_POSITION = glm::vec3(-3.0f, 9.0f, 0.0f);

class ClothCreator
{
public:
    std::vector<Cloth*> cloths;  // cloths parsed from .dxf file;
    const float step = STEP;     // steps between points

    ClothCreator(const std::string& clothFilePath) {
        readClothData(clothFilePath);
    }

    ~ClothCreator() {
        for (Cloth* c : cloths) {
            delete c;
        }
    }

private:
    glm::vec3 clothPos = CLOTH_POSITION;     // world position of cloth

    /*
     * dxf file parser
     */
    void readClothData(const std::string& clothFilePath) {
        std::cout << "Reading file " << clothFilePath << "...\n";

        Test_CreationClass* creationClass = new Test_CreationClass();
        DL_Dxf* dxf = new DL_Dxf();
        if (!dxf->in(clothFilePath, creationClass)) { // if file open failed
            std::cerr << clothFilePath << " could not be opened.\n";
            return;
        }

        createCloths(creationClass->blockNodes);

        delete dxf;
        delete creationClass;
    }

    /*
     * create Cloth using VERTEX data
     */
    void createCloths(std::vector<std::vector<point2D>>& clothNodes) {
        // use for loop to retrieve all cloths
        for (size_t i = 0, sz = clothNodes.size(); i < sz; i++) {
            // boundary of bounding box
            float minX = FLT_MAX, minY = FLT_MAX;
            float maxX = -FLT_MAX, maxY = -FLT_MAX;

            // Constrained Delaunay Triangulation(CDT)
            // ---------------------------------------
            CDT::Triangulation<float> cdt;
            std::vector<CDT::V2d<float>> contour;
            std::vector<CDT::Edge> edges;

            // add vertex into contour; contour should be closed
            for (size_t j = 0; j < clothNodes[i].size(); j++) {
                const point2D& p = clothNodes[i][j];
                contour.push_back({ p.first, p.second });
                updateBoundary(p, minX, maxX, minY, maxY);

                if (j == clothNodes[i].size() - 1) {
                    edges.push_back({ CDT::VertInd(0), CDT::VertInd(j) });
                }
                else {
                    edges.push_back({ CDT::VertInd(j), CDT::VertInd(j + 1) });
                }
            }

            // randomly generate vertex in the bounding box
            // if the vertex lies outside of contour, eraseOuterTrianglesAndHoles will erase it
            // add random offset to x, y to mimic real cloth
            Cloth* cloth = new Cloth(clothPos, minX, maxX, minY, maxY);
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

            // store index of nodes lying on the contour
            std::map<CDT::VertInd, int> contourIndex;
            for (const CDT::Edge& e : edges) {
                ++contourIndex[e.v1()];
                ++contourIndex[e.v2()];
            }

            // generate Node of Cloth
            for (size_t j = 0; j < cdt.vertices.size(); j++) {
                const CDT::V2d<float>& p = cdt.vertices[j];
                Node* n = new Node(p.x, p.y, 0.0f);
                n->lastWorldPosition = n->worldPosition = cloth->modelMatrix * glm::vec4(clothPos + n->localPosition, 1.0f);

                cloth->nodes.push_back(n);

                if (contourIndex.count(j)) {
                    cloth->sewNode.push_back(n);
                }
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
                /*cloth->springs.push_back(new Spring(n1, n2, cloth->shearCoef));
                cloth->springs.push_back(new Spring(n1, n3, cloth->shearCoef));
                cloth->springs.push_back(new Spring(n2, n3, cloth->shearCoef));
                cloth->springs.push_back(new Spring(n1, n2, cloth->bendingCoef));
                cloth->springs.push_back(new Spring(n1, n3, cloth->bendingCoef));
                cloth->springs.push_back(new Spring(n2, n3, cloth->bendingCoef));*/
            }
            std::cout << "Initialize cloth with " << cloth->nodes.size() << " nodes and " << cloth->faces.size() << " faces\n";

            cloths.push_back(cloth);

            // TODO: delete me
            // for debug purpose
            if (i == 1) break;
        }
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