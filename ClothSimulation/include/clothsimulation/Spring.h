#pragma once

#include "Point.h"

using namespace std;

class Spring
{
public:
	Node* node1;
	Node* node2;
	float hookCoef;    // 胡克系数
	float dampCoef;    // 阻尼系数
	float restLength;  // 弹簧不被拉伸时的长度

	Spring(Node* n1, Node* n2, float hookCoefficient)
	{
		node1 = n1;
		node2 = n2;
		hookCoef = hookCoefficient;
		dampCoef = 5.0;
		restLength = glm::length(node2->position - node1->position);
	}

	/*
	 * 计算弹簧的弹力, 并把弹力施加到顶点上
	 */
	void computeInternalForce(float timeStep)
	{
		float currentLength = glm::distance(node1->position, node2->position);
		glm::vec3 fDir1 = (node2->position - node1->position) / currentLength;
		glm::vec3 diffV1 = node2->velocity - node1->velocity;
		glm::vec3 f1 = fDir1 * ((currentLength - restLength) * hookCoef + glm::dot(diffV1, fDir1) * dampCoef);
		node1->addForce(f1);
		node2->addForce(-f1);
	}
};
