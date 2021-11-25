#pragma once

#include "Point.h"

using namespace std;

class Spring
{
public:
	Node* node1;
	Node* node2;
	double hookCoef;    // 胡克系数
	double dampCoef;    // 阻尼系数
	double restLength;  // 弹簧不被拉伸时的长度

	Spring(Node* n1, Node* n2, double hookCoefficient)
	{
		node1 = n1;
		node2 = n2;
		hookCoef = hookCoefficient;
		dampCoef = 5.0;
		restLength = (node2->position - node1->position).length();
	}

	/*
	 * 计算弹簧的弹力, 并把弹力施加到顶点上
	 */
	void computeInternalForce(double timeStep)
	{
		double currentLength = Vec3::dist(node1->position, node2->position);
		Vec3 fDir1 = (node2->position - node1->position) / currentLength;
		Vec3 diffV1 = node2->velocity - node1->velocity;
		Vec3 f1 = fDir1 * ((currentLength - restLength) * hookCoef + Vec3::dot(diffV1, fDir1) * dampCoef);
		node1->addForce(f1);
		node2->addForce(f1.minus());
	}
};
