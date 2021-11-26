#pragma once

#include <glm/glm.hpp>

// Default Point Values
const float MASS = 1.0;
const bool ISFIXED = false;
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
	bool		isFixed;		// Use to pin the cloth
	glm::vec2	texCoord;       // Texture coord
	glm::vec3	normal;         // For smoothly shading
	glm::vec3	position;
	glm::vec3   velocity;
	glm::vec3   force;
	glm::vec3	acceleration;

	Node(glm::vec3 pos = POSITION)
	{
		mass = MASS;
		isFixed = ISFIXED;
		position = pos;
		velocity = glm::vec3(0);
		force = glm::vec3(0);
		acceleration = glm::vec3(0);
	}
	~Node() {}

	void addForce(const glm::vec3 &force)
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
			acceleration = force / (float) mass;
			velocity += acceleration * timeStep;
			position += velocity * timeStep;
		}
		force = glm::vec3(0);
	}
};