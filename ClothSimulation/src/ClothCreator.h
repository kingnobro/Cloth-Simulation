#pragma once

#include <dxf/dl_dxf.h>
#include <dxf/dl_creationadapter.h>
#include "test_creationclass.h"

#include "Cloth.h"

// Defaults
glm::vec3 CLOTH_POSITION = glm::vec3(-3.0f, 9.0f, 0.0f);

class ClothCreator
{
public:
    std::vector<Cloth*> cloths;  // 从 dxf 文件中解析出的衣片. 一个 dxf 文件可以包含多个衣片

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

        // 解析文件中的顶点
        Test_CreationClass* creationClass = new Test_CreationClass();
        DL_Dxf* dxf = new DL_Dxf();
        if (!dxf->in(clothFilePath, creationClass)) { // if file open failed
            std::cerr << clothFilePath << " could not be opened.\n";
            return;
        }

        // 根据顶点数据创造布料对象
        std::vector<std::vector<Node*>>& clothNodes = creationClass->blockNodes;
        for (int i = 0, sz = clothNodes.size(); i < sz; i++) {
            Cloth* cloth = new Cloth(clothPos);
            std::vector<Node*>& nodes = clothNodes[i];

            // todo: learn face and spring generation algorithm
            for (Node* n : nodes) {
                n->lastWorldPosition = n->worldPosition = clothPos + n->localPosition;
                // std::cout << n->worldPosition.x << " " << n->worldPosition.y << " " << n->worldPosition.z << "\n";
                cloth->faces.push_back(n);
                cloth->nodes.push_back(n);
            }
            for (int i = 1, sz = nodes.size(); i < sz; i++) {

                cloth->springs.push_back(new Spring(nodes[i], nodes[i - 1], cloth->structuralCoef));

            }
            //cloth->springs.push_back(new Spring(nodes[0], nodes[1], cloth->structuralCoef));
            std::cout << "Initialize cloth with " << cloth->nodes.size() << " nodes and " << cloth->faces.size() << " faces\n";

            cloths.push_back(cloth);
        }

        delete dxf;
        delete creationClass;
	}
};