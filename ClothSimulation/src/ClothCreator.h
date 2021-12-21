#ifndef CLOTHCREATOR_H
#define CLOTHCREATOR_H

#include <map>
#include <CDT/CDT.h>
#include <dxf/dl_dxf.h>
#include <dxf/dl_creationadapter.h>
#include "test_creationclass.h"

#include "Cloth.h"

// Defaults
const float STEP = 20.0f;
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

        // a dxf file may have multiple cloths, thus use for loop to retrieve all cloths
        for (size_t i = 0, clth_sz = clothNodes->size(); i < clth_sz; i++) {

            // boundary of bounding box
            float minX = FLT_MAX, minY = FLT_MAX;
            float maxX = -FLT_MAX, maxY = -FLT_MAX;

            // Constrained Delaunay Triangulation(CDT)
            // ---------------------------------------
            // initialize data structure
            CDT::Triangulation<float> cdt;
            std::vector<CDT::V2d<float>> vertices;
            std::vector<CDT::Edge> edges;

            // add contour points into vertices; contour should be closed
            for (size_t j = 0, ctr_sz = (*clothNodes)[i].size(); j < ctr_sz; j++) {
                const point2D& p = (*clothNodes)[i][j];
                vertices.push_back({ p.first, p.second });

                updateBoundary(p, minX, maxX, minY, maxY);

                if (j == ctr_sz - 1) {
                    edges.push_back({ CDT::VertInd(0), CDT::VertInd(j) });
                }
                else {
                    edges.push_back({ CDT::VertInd(j), CDT::VertInd(j + 1) });  // small index should come first
                }
            }

            std::cout << "contour size: " << vertices.size() << "\n";
            std::cout << "minX: " << minX
                << " maxX: " << maxX
                << " minY: " << minY
                << " maxY: " << maxY
                << std::endl;

            // add vertex in the bounding box to generate triangle mesh
            // if the vertex lies outside of contour, eraseOuterTrianglesAndHoles will erase it
            for (float x = minX; x < maxX; x += step) {
                for (float y = minY; y < maxY; y += step) {
                    // there we add regular arranged points, because it's easier to create springs under this circumstances
                    vertices.push_back({ x, y });
                }
            }

            // triangulation
            CDT::RemoveDuplicatesAndRemapEdges(vertices, edges);
            cdt.insertVertices(vertices);
            cdt.insertEdges(edges);
            cdt.eraseOuterTrianglesAndHoles();

            // create a cloth
            Cloth* cloth = new Cloth(clothPos, minX, maxX, minY, maxY);
            createCloth(cdt, cloth, minX, maxX, minY, maxY);
            cloths.push_back(cloth);

            // TODO: delete me
            // for debug purpose
            if (i == 1) break;
        }
    }

    /*
     * create a cloth
     * 1. �� cdt.vertices ���� Cloth �Ķ���
     * 2. ���������ҵ��յ�
     * 3. ���ùյ㽫�����ֳ� segments
     * 4. ������ֵ���: structural, shear, bending
     */
    void createCloth(CDT::Triangulation<float>& cdt, Cloth* cloth, float minX, float maxX, float minY, float maxY) {
        // create Nodes of Cloth from 2D points
        std::map<int, Node*> idOfNode;  // ��¼������ӵ���Ƭ�еĵ�� id, id ����������������
        int nodesPerRow = round((maxX - minX) / step);  // ���ڸ����������, ��Ҫ��������, �������� 1
        int nodesPerCol = round((maxY - minY) / step);

        // triangulation ֮��, ����ĵ㲢���ᱻ�Ƴ��� cdt.vertices
        // ����ֻ�ܴ� cdt.triangles ���ҵ����б��õ��ĵ���±�
        std::map<int, Node*> indexOfNode;   // ��¼�±�, ���������ӵ�
        std::map<CDT::VertInd, int> contourIndex;   // store index of nodes lying on the contour
        for (const CDT::Edge& e : cdt.fixedEdges) {
            ++contourIndex[e.v1()];
            ++contourIndex[e.v2()];
        }
        // �Ȱ������ϵĵ�ȫ������; ���������һ��ѭ���м���, �����ϵĵ������, �޷���˳ʱ�����
        for (std::map<CDT::VertInd, int>::iterator it = contourIndex.begin(); it != contourIndex.end(); it++) {
            int index = it->first;
            const CDT::V2d<float>& p = cdt.vertices[index];
            Node* n = newNodeFromIndex(p, cloth, index);
            cloth->nodes.push_back(n);
            cloth->contour.push_back(n);
            indexOfNode[index] = n;
        }
        for (const CDT::Triangle& tri : cdt.triangles) {
            // ������������������
            for (int i = 0; i < 3; i++) {
                int index = tri.vertices[i];
                Node* n = nullptr;
                if (!indexOfNode.count(index)) {    // ������ǵ�һ�γ���
                    const CDT::V2d<float>& p = cdt.vertices[index];
                    n = newNodeFromIndex(p, cloth, index);
                    // �Ѿ���ӹ������ϵĵ���, ���� id != -1
                    // (p.x-minX) �� (p.y-minY) �ض��� step �ı���, ���Կ����� round ȡ��
                    // ������ int, ����־�������
                    n->id = round((n->localPosition.x - minX) / step) + nodesPerRow * round((n->localPosition.y - minY) / step);
                    cloth->nodes.push_back(n);
                    indexOfNode[index] = n;
                    idOfNode[n->id] = n;
                }
                else {  // �������ֹ���
                    n = indexOfNode[index];
                }
                cloth->faces.push_back(n);
            }
        }

        // Find Turning Nodes �ҵ��յ�
        // ------------------
        // for every node 'middle' in contour, find its neighbors 'prev' and 'next'
        // we can get 2 vectors: v1 = middle - prev, v2 = next - middle
        // if theta between (n1, n2) > threshold, then we take 'middle' as a 'turning point'
        Node* prev, * middle, * next;
        int index;  // index of one of the turning point
        for (int j = 0, ctr_sz = cloth->contour.size(); j < ctr_sz; j++) {
            middle = cloth->contour[j];
            prev = cloth->contour[(j - 1 + ctr_sz) % ctr_sz];
            next = cloth->contour[(j + 1) % ctr_sz];

            glm::vec3 v1 = middle->worldPosition - prev->worldPosition;
            glm::vec3 v2 = next->worldPosition - middle->worldPosition;
            float len1 = glm::length(v1);
            float len2 = glm::length(v2);
            assert(len1 * len2 > 1e-5);  // avoid dividing by zero

            float cos = glm::dot(v1, v2) / (len1 * len2);
            middle->isTurningPoint = glm::dot(v1, v2) < 0 || (cos < glm::cos(20));
            if (middle->isTurningPoint) {
                index = j;  // find a turning point, for segments generation 
            }
        }

        // Generate Contour Segments
        // -------------------------
        // divide contour into several segments
        // each segments have plenty of points; points in the same segment have same segmentID
        // ��������*������*������������
        // ��             ��
        // ����������*��������*��������
        std::vector<Node*> segment;
        int segmentID = 0;
        for (int cnt = 0, ctr_sz = cloth->contour.size(); cnt <= ctr_sz; ++cnt) {
            Node* n = cloth->contour[(index + cnt) % ctr_sz];
            if (n->isTurningPoint && cnt != 0) {
                segment.push_back(n);   // add turning points of two end; potential bug exists
                cloth->segments.push_back(segment);
                segment.clear();
                segmentID += 1;
            }
            n->segmentID = segmentID;
            segment.push_back(n);
        }


        // Generate Springs
        // ----------------------------------------------
        std::map<std::pair<int, int>, int> springExist;
        for (const CDT::Triangle& tri : cdt.triangles) {
            Node* n1 = indexOfNode[tri.vertices[0]];
            Node* n2 = indexOfNode[tri.vertices[1]];
            Node* n3 = indexOfNode[tri.vertices[2]];
            int id1 = n1->id;
            int id2 = n2->id;
            int id3 = n3->id;

            cloth->springs.push_back(new Spring(n1, n2, cloth->structuralCoef + 50.0f));
            cloth->springs.push_back(new Spring(n1, n3, cloth->structuralCoef));
            cloth->springs.push_back(new Spring(n2, n3, cloth->structuralCoef - 50.0f));
            // store which two nodes have springs between them already
            if (id1 != -1 && id2 != -1) {
                ++springExist[{std::min(id1, id2), std::max(id1, id2)}];
            }
            if (id1 != -1 && id3 != -1) {
                ++springExist[{std::min(id1, id3), std::max(id1, id3)}];
            }
            if (id2 != -1 && id3 != -1) {
                ++springExist[{std::min(id2, id3), std::max(id2, id3)}];
            }
        }
        // add shear springs on mesh nodes
        for (int j = 0, node_sz = cloth->nodes.size(); j < node_sz; j++) {
            Node* n = cloth->nodes[j];
            int id = n->id;
            if (id < 0) {
                continue;
            }

            // id of diagonal nodes
            int x1 = round((n->localPosition.x + step - minX) / step);
            int x2 = x1 - 2;
            int x3 = x1 - 1;
            int x4 = x1 + 1;
            int y1 = round((n->localPosition.y + step - minY) / step);
            int y2 = y1;
            int y3 = y1 + 1;
            int y4 = y1 - 1;
            addSpring(cloth, n, id, x1, y1, cloth->shearCoef, nodesPerRow, nodesPerCol, idOfNode, springExist);   // ���Ͻ�
            addSpring(cloth, n, id, x2, y2, cloth->shearCoef, nodesPerRow, nodesPerCol, idOfNode, springExist);   // ���½�
            addSpring(cloth, n, id, x3, y3, cloth->shearCoef, nodesPerRow, nodesPerCol, idOfNode, springExist);   // ���Ϸ�2����λ
            addSpring(cloth, n, id, x4, y4, cloth->shearCoef, nodesPerRow, nodesPerCol, idOfNode, springExist);   // ���ҷ�2����λ
        }
        // add bending springs on contour
        for (int j = 0, ctr_sz = cloth->contour.size(); j < ctr_sz; j++) {
            Node* n1 = cloth->contour[j];
            Node* n2 = cloth->contour[(j + 2) % ctr_sz];
            cloth->springs.push_back(new Spring(n1, n2, cloth->bendingCoef));
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

    Node* newNodeFromIndex(const CDT::V2d<float>& position, Cloth* cloth, int index) {
        Node* n = new Node(position.x, position.y, 0.0f);
        n->lastWorldPosition = n->worldPosition = cloth->modelMatrix * glm::vec4(n->localPosition, 1.0f);
        return n;
    }
    
    // ���� x, y ��ȡ�ʵ� id, �����ӵ���
    void addSpring(
        Cloth* cloth, 
        Node* n, int id,
        int x, int y, float coef,
        int nodesPerRow, int nodesPerCol,
        std::map<int, Node*>& idOfNode,
        std::map<std::pair<int, int>, int>& springExist) {
        if (x >= 0 && x < nodesPerRow && y >= 0 && y < nodesPerCol) {
            int id1 = x + nodesPerRow * y;
            if (idOfNode.count(id1) && !springExist.count({ std::min(id1, id), std::max(id1, id) })) {
                Node* n1 = idOfNode[id1];
                cloth->springs.push_back(new Spring(n, n1, coef));
                ++springExist[{std::min(id1, id), std::max(id1, id)}];
            }
        }
    }
};

#endif