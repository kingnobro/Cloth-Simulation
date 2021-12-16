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
    std::vector<Cloth*> cloths;  // 从 dxf 文件中解析出的衣片. 一个 dxf 文件可以包含多个衣片
    const float step = STEP;    // 生成点的步长

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
    glm::vec3 clothPos;     // 衣片的世界坐标

    /*
     * dxf 文件解析
     */
    void readClothData(const std::string& clothFilePath) {
        std::cout << "Reading file " << clothFilePath << "...\n";

        // 使用 dxflib 解析文件中的顶点
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
     * 根据解析出的 Vertex 数据创造 Cloth 对象
     */
    void createCloths(std::vector<std::vector<point2D>>& clothNodes) {
        // 一个 dxf 文件可能包含多个衣片, 所以用一个循环取出所有的衣片
        for (size_t i = 0, sz = clothNodes.size(); i < sz; i++) {
            Cloth* cloth = new Cloth(clothPos);
            float minX =  FLT_MAX, minY =  FLT_MAX;   // 用于生成衣片的包围盒
            float maxX = -FLT_MAX, maxY = -FLT_MAX;

            // 三角网格化, Constrained Delaunay Triangulation(CDT)
            CDT::Triangulation<float> cdt;
            std::vector<CDT::V2d<float>> contour;
            std::vector<CDT::Edge> edges;

            // 把轮廓上的点加入三角化的类中, 并添加轮廓线, 避免产生凸包; 轮廓线需要闭合
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

            // 在衣片的矩形包围盒内随机生成点, 假如生成的点落在衣片轮廓外部, 它会被 eraseOuterTrianglesAndHoles 删除
            // 添加随机偏移, 使得生成的三角形网格更加接近面料
            cloth->width = int(maxX - minX);
            cloth->height = int(maxY - minY);
            int round = 0;
            for (float x = minX; x < maxX; x += step) {
                for (float y = minY; y < maxY; y += step) {
                    // 随机偏移的参数是随便设置的
                    contour.push_back({ x + (round % 7) * 0.7f, y - (round % 7) * 1.0f });
                    round += 1;
                }
            }

            cdt.insertVertices(contour);
            cdt.insertEdges(edges);
            cdt.eraseOuterTrianglesAndHoles();  // 抹去边框外和 hole 中的三角形与顶点

            // 在此处生成衣片上的点, 因为经过 eraseOuterTrianglesAndHoles 后在轮廓外的点会被 erase
            for (const CDT::V2d<float>& p : cdt.vertices) {
                Node* n = new Node(p.x, p.y, 0.0f);
                n->lastWorldPosition = n->worldPosition = clothPos + n->localPosition;
                cloth->nodes.push_back(n);
            }

            // 取出三角形, 生成 springs 和 faces 数据
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
            // debug 时用 break 控制衣片生成的数量
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