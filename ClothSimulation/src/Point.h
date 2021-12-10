#pragma once

#include <glm/glm.hpp>

// Default Point Values
const float MASS = 1.0;
const glm::vec3 POSITION = glm::vec3(0);

/*
 * 用于表示面(face)上的顶点, 需要同时有位置和法线向量
 */
class Vertex
{
public:
    glm::vec3 position;
    glm::vec3 normal;

    Vertex() {}
    Vertex(glm::vec3 pos)
    {
        position = pos;
    }
    ~Vertex() {}
};

class Node
{
public:
    float		mass;
    bool		isFixed;		// Use to pin the cloth
    bool        isSewed;        // 判断该点是否已缝合
    glm::vec2	texCoord;       // Texture coord
    glm::vec3	normal;         // For smoothly shading
    glm::vec3   localPosition;  // 局部坐标, 用于恢复服装的原始位置
    glm::vec3   worldPosition;  // 世界坐标, 用于计算弹簧受力
    glm::vec3   lastWorldPosition;	// 质点前一时刻的位置, 用于碰撞响应
    glm::vec3   velocity;
    glm::vec3   force;
    glm::vec3	acceleration;

    Node(glm::vec3 pos = POSITION)
    {
        mass = MASS;
        isFixed = false;
        isSewed = false;
        localPosition = pos;
        worldPosition = glm::vec3(0);
        lastWorldPosition = glm::vec3(0);
        velocity = glm::vec3(0);
        force = glm::vec3(0);
        acceleration = glm::vec3(0);
    }
    ~Node() {}

    void addForce(const glm::vec3& force)
    {
        this->force += force;
    }

    /*
     * calculate movement of point
     * only non-fixed points get integrated
     */
    void integrate(float timeStep)
    {
        // Verlet integration
        if (!isFixed)
        {
            // Newton's second law of motion
            acceleration = force / mass;
            velocity += acceleration * timeStep;
            lastWorldPosition = worldPosition;
            worldPosition += velocity * timeStep;
        }
        force = glm::vec3(0);
    }
};