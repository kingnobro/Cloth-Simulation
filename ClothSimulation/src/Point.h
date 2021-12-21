#ifndef POINT_H
#define POINT_H

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
    int         segmentID;      // 衣片边缘被分为很多段, 该点所属的段 ID; 若不是边缘上的点则置为 -1
    int         meshId;         // 如果该点在轮廓上 meshId = -1, 这个字段用于寻找该点周围的点, 从而在点之间生成弹簧
    int         globalID;       // 用于布料自碰撞的检测
    bool        isSewed;        // 判断该点是否已缝合
    bool        isSelected;     // 判断是否已经被选为缝合点
    bool        isTurningPoint; // 判断该点是否为缝合线上的转折点
    glm::vec2	texCoord;       // Texture coord
    glm::vec3	normal;         // For smoothly shading
    glm::vec3   localPosition;  // 局部坐标, 仅用于恢复服装的原始位置
    glm::vec3   worldPosition;  // 世界坐标, 用于计算弹簧受力
    glm::vec3   lastWorldPosition;	// 质点前一时刻的世界坐标, 用于碰撞响应
    glm::vec3   velocity;
    glm::vec3   force;
    glm::vec3	acceleration;

    Node(glm::vec3 pos = POSITION)
    {
        localPosition = pos;
        init();
    }
    Node(float x, float y, float z)
    {
        localPosition = glm::vec3(x, y, z);
        init();
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
        acceleration = force / mass;
        velocity += acceleration * timeStep;
        lastWorldPosition = worldPosition;
        worldPosition += velocity * timeStep;
        force = glm::vec3(0);
    }

    void reset()
    {
        isSewed = isSelected = false;
        velocity = acceleration = force = glm::vec3(0);
    }

private:
    void init()
    {
        mass = MASS;
        segmentID = -1; // 默认不是边缘上的点
        meshId = -1;
        globalID = -1;
        isSewed = false;
        isSelected = false;
        worldPosition = glm::vec3(0);
        lastWorldPosition = glm::vec3(0);
        velocity = glm::vec3(0);
        force = glm::vec3(0);
        acceleration = glm::vec3(0);
    }
};

#endif