#pragma once

#include "Point.h"

using namespace std;

class Spring
{
public:
	Node* node1;
	Node* node2;
	float hookCoef;    // ����ϵ��
	float dampCoef;    // ����ϵ��
	float restLength;  // ���ɲ�������ʱ�ĳ���

	Spring(Node* n1, Node* n2, float hookCoefficient)
	{
		node1 = n1;
		node2 = n2;
		hookCoef = hookCoefficient;
		dampCoef = 5.0;
		restLength = glm::distance(node1->position, node2->position);
	}

	/*
	 * ���㵯�ɵĵ���, ���ѵ���ʩ�ӵ��ʵ���
	 * ���������ʵ������ ��С��ͬ, �����෴
	 */
	void computeInternalForce(float timeStep)
	{
		float currentLength = glm::distance(node1->position, node2->position);
		glm::vec3 forceDirection = (node2->position - node1->position) / currentLength;
		glm::vec3 velocityDifference = node2->velocity - node1->velocity;
		glm::vec3 force = forceDirection * ((currentLength - restLength) * hookCoef + glm::dot(velocityDifference, forceDirection) * dampCoef);
		node1->addForce(force);
		node2->addForce(-force);
	}
};
