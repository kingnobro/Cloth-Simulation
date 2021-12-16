#pragma once

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
    std::vector<Cloth*> cloths;  // �� dxf �ļ��н���������Ƭ. һ�� dxf �ļ����԰��������Ƭ
    const float step = STEP;    // ���ɵ�Ĳ���

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

        // ʹ�� dxflib �����ļ��еĶ���
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
     * ���ݽ������� Vertex ���ݴ��� Cloth ����
     */
    void createCloths(std::vector<std::vector<point2D>>& clothNodes) {
        // һ�� dxf �ļ����ܰ��������Ƭ, ������һ��ѭ��ȡ�����е���Ƭ
        for (size_t i = 0, sz = clothNodes.size(); i < sz; i++) {
            Cloth* cloth = new Cloth(clothPos);
            float minX =  FLT_MAX, minY =  FLT_MAX;   // ����������Ƭ�İ�Χ��
            float maxX = -FLT_MAX, maxY = -FLT_MAX;

            // ��������, Constrained Delaunay Triangulation(CDT)
            CDT::Triangulation<float> cdt;
            std::vector<CDT::V2d<float>> contour;
            std::vector<CDT::Edge> edges;

            // �������ϵĵ�������ǻ�������, �����������, �������͹��; ��������Ҫ�պ�
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

            // ����Ƭ�ľ��ΰ�Χ����������ɵ�, �������ɵĵ�������Ƭ�����ⲿ, ���ᱻ eraseOuterTrianglesAndHoles ɾ��
            // ������ƫ��, ʹ�����ɵ�������������ӽӽ�����
            cloth->width = int(maxX - minX);
            cloth->height = int(maxY - minY);
            int round = 0;
            for (float x = minX; x < maxX; x += step) {
                for (float y = minY; y < maxY; y += step) {
                    // ���ƫ�ƵĲ�����������õ�
                    contour.push_back({ x + (round % 7) * 0.7f, y - (round % 7) * 1.0f });
                    round += 1;
                }
            }

            cdt.insertVertices(contour);
            cdt.insertEdges(edges);
            cdt.eraseOuterTrianglesAndHoles();  // Ĩȥ�߿���� hole �е��������붥��

            // �ڴ˴�������Ƭ�ϵĵ�, ��Ϊ���� eraseOuterTrianglesAndHoles ����������ĵ�ᱻ erase
            for (const CDT::V2d<float>& p : cdt.vertices) {
                Node* n = new Node(p.x, p.y, 0.0f);
                n->lastWorldPosition = n->worldPosition = clothPos + n->localPosition;
                cloth->nodes.push_back(n);
            }

            // ȡ��������, ���� springs �� faces ����
            Node* n1, * n2, * n3;
            for (const CDT::Triangle& tri : cdt.triangles) {
                CDT::VerticesArr3 triIndex = tri.vertices;
                n1 = cloth->nodes[triIndex[0]];
                n2 = cloth->nodes[triIndex[1]];
                n3 = cloth->nodes[triIndex[2]];
                cloth->faces.push_back(n1);
                cloth->faces.push_back(n2);
                cloth->faces.push_back(n3);
                cloth->springs.push_back(new Spring(n1, n2, cloth->structuralCoef));
                cloth->springs.push_back(new Spring(n1, n3, cloth->structuralCoef));
                cloth->springs.push_back(new Spring(n2, n3, cloth->structuralCoef));
            }
            std::cout << "Initialize cloth with " << cloth->nodes.size() << " nodes and " << cloth->faces.size() << " faces\n";

            cloths.push_back(cloth);

            // todo: delete me
            // debug ʱ�� break ������Ƭ���ɵ�����
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