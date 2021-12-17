#pragma once

#include "Point.h"

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
        restLength = glm::distance(node1->worldPosition, node2->worldPosition);
    }

    /*
     * ���㵯�ɵĵ���, ���ѵ���ʩ�ӵ��ʵ���
     * ���������ʵ������ ��С��ͬ, �����෴
     */
    void computeInternalForce(float timeStep)
    {
        float currentLength = glm::distance(node1->worldPosition, node2->worldPosition);
        
        glm::vec3 forceDirection = (node2->worldPosition - node1->worldPosition) / currentLength;
        glm::vec3 velocityDifference = node2->velocity - node1->velocity;
        glm::vec3 force = forceDirection * ((currentLength - restLength) * hookCoef + glm::dot(velocityDifference, forceDirection) * dampCoef);

        node1->addForce(force);
        node2->addForce(-force);
    }

    /*
     * constraining super-elasticity
     * After each iteration it checks for each spring whether it exceeds its natural length by a pre-given threshold
     * If this is the case, the velocities are modified, so that further elongation is not allowed
     * Let p1 and p2 be the positions of the end points of a spring found as over-elongated
     * Decompose v1 and v2, and set vt to zero to avoid over-stretching
     */
    // void velocityModification() {}
};
