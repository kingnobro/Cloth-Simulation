#ifndef SPRING_H
#define SPRING_H

#include "Point.h"

class Spring
{
public:
    Node* node1;
    Node* node2;
    float hookCoef;    // 'k' in Hooke's law
    float dampCoef;    // avoid continuous dang swings
    float restLength;  // length of spring when it is rest

    Spring(Node* n1, Node* n2, float hookCoefficient)
    {
        node1 = n1;
        node2 = n2;
        hookCoef = hookCoefficient;
        dampCoef = 2;
        restLength = glm::distance(node1->worldPosition, node2->worldPosition);
    }

    /*
     * computer force of a spring
     * ¡û¡ð~~~¡ð¡ú      compressed
     * ¡ð¡ú~~~~~~~~¡û¡ð stretched
     * force and velocity of two nodes have opposite directions
     */
    void computeInternalForce(float timeStep)
    {
        float currentLength = glm::distance(node1->worldPosition, node2->worldPosition);
        // restrain min length; otherwise force will be very large
        // currentLength = std::max(currentLength, restLength / 50);
        // todo: currentLength should have upper limit
        
        glm::vec3 forceDirection = (node2->worldPosition - node1->worldPosition) / currentLength;
        glm::vec3 velocityDifference = node2->velocity - node1->velocity;
        glm::vec3 force = forceDirection * ((currentLength - restLength) * hookCoef + glm::dot(velocityDifference, forceDirection) * dampCoef);

        node1->addForce(force);
        node2->addForce(-force);
        //velocityModification();
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

#endif
