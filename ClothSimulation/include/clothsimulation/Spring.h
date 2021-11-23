#pragma once

#include "Points.h"

using namespace std;

class Spring
{
public:
	Node* node1;
	Node* node2;
	double restLength;  // ���ɲ�������ʱ�ĳ���
	double hookCoef;    // ����ϵ��
	double dampCoef;    // ����ϵ��

	// k Ϊ����ϵ��
	Spring(Node* n1, Node* n2, double hookCoefficient)
	{
		node1 = n1;
		node2 = n2;

		Vec3 springVec = node2->position - node1->position;
		restLength = springVec.length();
		hookCoef = hookCoefficient;
		dampCoef = 5.0;
	}

	void applyInternalForce(double timeStep) // Compute spring internal force
	{
		double currLength = Vec3::dist(node1->position, node2->position);
		Vec3 fDir1 = (node2->position - node1->position) / currLength;
		Vec3 diffV1 = node2->velocity - node1->velocity;
		Vec3 f1 = fDir1 * ((currLength - restLength) * hookCoef + Vec3::dot(diffV1, fDir1) * dampCoef);
		node1->addForce(f1);
		node2->addForce(f1.minus());
	}
};
