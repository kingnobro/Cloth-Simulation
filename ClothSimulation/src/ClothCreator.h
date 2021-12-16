#pragma once

#include <CDT/CDT.h>
#include <dxf/dl_dxf.h>
#include <dxf/dl_creationadapter.h>
#include "test_creationclass.h"

#include "Cloth.h"

// Defaults
glm::vec3 CLOTH_POSITION = glm::vec3(-3.0f, 9.0f, 0.0f);

class ClothCreator
{
public:
    std::vector<Cloth*> cloths;  // �� dxf �ļ��н���������Ƭ. һ�� dxf �ļ����԰��������Ƭ

	ClothCreator(const std::string& clothFilePath) {
        clothPos = CLOTH_POSITION;

        readClothData(clothFilePath);
	}

    ~ClothCreator() {
        for (Cloth* c : cloths) {
            delete c;
        }
    }

private:
    glm::vec3 clothPos;     // ��Ƭ����������

    /*
     * dxf �ļ�����
     */
    void readClothData(const std::string& clothFilePath) {
        std::cout << "Reading file " << clothFilePath << "...\n";

        // �����ļ��еĶ���
        Test_CreationClass* creationClass = new Test_CreationClass();
        DL_Dxf* dxf = new DL_Dxf();
        if (!dxf->in(clothFilePath, creationClass)) { // if file open failed
            std::cerr << clothFilePath << " could not be opened.\n";
            return;
        }

        // ���� Vertex ���ݴ��� Cloth ����
        std::vector<std::vector<point2D>>& clothNodes = creationClass->blockNodes;
        for (size_t i = 0, sz = clothNodes.size(); i < sz; i++) {
            Cloth* cloth = new Cloth(clothPos);

            // Constrained Delaunay Triangulation
            CDT::Triangulation<float> cdt;
            std::vector<CDT::V2d<float>> vertices;
            std::vector<CDT::Edge> edges;
            for (size_t j = 0; j < clothNodes[i].size(); j++) {
                const point2D& p = clothNodes[i][j];
                vertices.push_back({ p.first, p.second });

                // ���������, �������͹��. ��������Ҫ�պ�
                if (j == clothNodes[i].size() - 1) {
                    edges.push_back({ CDT::VertInd(0), CDT::VertInd(j) });
                }
                else {
                    edges.push_back({ CDT::VertInd(j), CDT::VertInd(j + 1) });
                }

                // ��Ҫ���Ƶĵ�
                // todo: ��� z ����
                Node* n = new Node(p.first, p.second, 0.0f);
                n->lastWorldPosition = n->worldPosition = clothPos + n->localPosition;
                cloth->nodes.push_back(n);
            }
            cdt.insertVertices(vertices);
            cdt.insertEdges(edges);
            cdt.eraseOuterTrianglesAndHoles();  // Ĩȥ�߿���� hole �е�������

            // ȡ��������, ���� springs �� faces ����
            Node* n1, * n2, * n3;
            for (const CDT::Triangle& triangle : cdt.triangles) {
                CDT::VerticesArr3 tri = triangle.vertices;
                n1 = cloth->nodes[tri[0]];
                n2 = cloth->nodes[tri[1]];
                n3 = cloth->nodes[tri[2]];
                cloth->faces.push_back(n1);
                cloth->faces.push_back(n2);
                cloth->faces.push_back(n3);
                cloth->springs.push_back(new Spring(n1, n2, cloth->structuralCoef));
                cloth->springs.push_back(new Spring(n1, n3, cloth->structuralCoef));
                cloth->springs.push_back(new Spring(n2, n3, cloth->structuralCoef));
            }
            std::cout << "Initialize cloth with " << cloth->nodes.size() << " nodes and " << cloth->faces.size() << " faces\n";

            cloths.push_back(cloth);
        }

        delete dxf;
        delete creationClass;
	}
};