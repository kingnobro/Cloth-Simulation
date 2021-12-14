#pragma once

#include <dxf/dl_dxf.h>
#include <dxf/dl_creationadapter.h>
#include "test_creationclass.h"

#include "Cloth.h"

glm::vec3 CLOTH_POSITION = glm::vec3(-3.0f, 9.0f, 0.0f);

class ClothCreator
{
public:
    std::vector<Node*> nodes;    // 存放衣片的顶点
    std::vector<Cloth*> cloths;  // 从 dxf 文件中解析出的衣片. 一个 dxf 文件可以包含多个衣片

	ClothCreator(const std::string& clothFilePath) {
        clothPos = CLOTH_POSITION;
		readClothData(clothFilePath);
	}

    void createCloth()
    {
        Cloth cloth = Cloth(clothPos);
        cloth.nodes.swap(nodes);        // 将 nodes 中的数据全部移到 cloth.nodes 中, 此时 nodes 中的数据被清空
        assert(nodes.size() == 0);

        // todo: learn face and spring generation algorithm
        for (Node* n : nodes) {
            n->lastWorldPosition = n->worldPosition = clothPos + n->localPosition;
            cloth.faces.push_back(n);
        }
        cloth.springs.push_back(new Spring(nodes[0], nodes[1], cloth.structuralCoef));
        std::cout << "Initialize cloth with " << cloth.nodes.size() << " nodes and " << cloth.faces.size() << " faces\n";

        cloths.push_back(&cloth);
    }

private:
    glm::vec3 clothPos;     // 衣片的世界坐标

    /*
     * dxf 文件解析
     */
	void readClothData(const std::string& clothFilePath) {
        std::cout << "Reading file " << clothFilePath << "...\n";

        Test_CreationClass* creationClass = new Test_CreationClass();
        DL_Dxf* dxf = new DL_Dxf();
        if (!dxf->in(clothFilePath, creationClass)) { // if file open failed
            std::cerr << clothFilePath << " could not be opened.\n";
            return;
        }

        delete dxf;
        delete creationClass;
	}
};