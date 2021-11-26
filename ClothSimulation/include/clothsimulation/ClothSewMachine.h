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
	 * �����Ƭ
	 * ����ֱ���޸� Cloth �� clothPos, ��Ϊ�޸� closhPos �ᵼ�� modelMatrix �ı�
	 * ������Ч����, ��һ����Ⱦѭ����, ��Ƭ˲���ƶ�
	 * ������Ҫֱ���޸� Node �ľֲ�����
	 * Cloth ֻ�ܷ��һ��
	 */
	void SewCloths()
	{
		glm::vec3 delta = glm::abs(cloth1->clothPos - cloth2->clothPos) / (float)2;

		// ��Ҫ�޸� Node �ľֲ�����
		// ��Ϊ Cloth ֻ�ܷ��һ��, ��ζ�� Node �ľֲ�����ֻ���޸�һ��, �������������ҵ�����
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