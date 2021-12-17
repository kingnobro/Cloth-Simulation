#ifndef POINT_H
#define POINT_H

#include <glm/glm.hpp>

// Default Point Values
const float MASS = 1.0;
const glm::vec3 POSITION = glm::vec3(0);

/*
 * ���ڱ�ʾ��(face)�ϵĶ���, ��Ҫͬʱ��λ�úͷ�������
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
    bool        isSewed;        // �жϸõ��Ƿ��ѷ��
    glm::vec2	texCoord;       // Texture coord
    glm::vec3	normal;         // For smoothly shading
    glm::vec3   localPosition;  // �ֲ�����, �����ڻָ���װ��ԭʼλ��
    glm::vec3   worldPosition;  // ��������, ���ڼ��㵯������
    glm::vec3   lastWorldPosition;	// �ʵ�ǰһʱ�̵���������, ������ײ��Ӧ
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
        isSewed = false;
        velocity = acceleration = force = glm::vec3(0);
    }

private:
    void init()
    {
        mass = MASS;
        isSewed = false;
        worldPosition = glm::vec3(0);
        lastWorldPosition = glm::vec3(0);
        velocity = glm::vec3(0);
        force = glm::vec3(0);
        acceleration = glm::vec3(0);
    }
};

#endif