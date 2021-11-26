#pragma once

#include <glm/glm.hpp>
#include "Cloth.h"

class ClothSewMachine
{
public:
	Cloth* cloth1;
	Cloth* cloth2;

	ClothSewMachine(Cloth* cloth1, Cloth* cloth2)
	{
		this->cloth1 = cloth1;
		this->cloth2 = cloth2;
	}

	/*
	 * 缝合衣片
	 * 不能直接修改 Cloth 的 clothPos, 因为修改 closhPos 会导致 modelMatrix 改变
	 * 这样的效果是, 下一个渲染循环中, 衣片瞬间移动
	 * 所以需要直接修改 Node 的局部坐标
	 * Cloth 只能缝合一次
	 */
	void SewCloths()
	{
		glm::vec3 delta = glm::abs(cloth1->clothPos - cloth2->clothPos) / (float)2;

		// 需要修改 Node 的局部坐标
		// 因为 Cloth 只能缝合一次, 意味着 Node 的局部坐标只会修改一次, 不会出现坐标错乱的问题
		for (int i = 0; i < cloth1->nodesPerRow; i++) {
			Node* n1 = cloth1->getNode(i, 0);
			Node* n2 = cloth2->getNode(i, 0);
		
			if (cloth1->getWorldPos(n1).x > cloth2->getWorldPos(n2).x) {
				n1->position.x -= delta.x;
				n2->position.x += delta.x;
			} else {
				n1->position.x += delta.x;
				n2->position.x -= delta.x;
			}
		
			if (cloth1->getWorldPos(n1).y > cloth2->getWorldPos(n2).y) {
				n1->position.y -= delta.y;
				n2->position.y += delta.y;
			}
			else {
				n1->position.y += delta.y;
				n2->position.y -= delta.y;
			}
		
			if (cloth1->getWorldPos(n1).z > cloth2->getWorldPos(n2).z) {
				n1->position.z -= delta.z;
				n2->position.z += delta.z;
			}
			else {
				n1->position.z += delta.z;
				n2->position.z -= delta.z;
			}
		}

	}
};