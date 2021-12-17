#pragma once

#include "Point.h"

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
        restLength = glm::distance(node1->worldPosition, node2->worldPosition);
    }

    /*
     * 计算弹簧的弹力, 并把弹力施加到质点上
     * 弹簧两个质点的受力 大小相同, 方向相反
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
