#pragma once

#include "Vector.h"

// Default Point Values
const double MASS = 1.0;
const bool ISFIXED = false;
const Vec3 POSITION = Vec3(0, 0, 0);

struct Vertex
{
public:
	Vec3 position;
	Vec3 normal;

	Vertex() {}
	Vertex(Vec3 pos)
	{
		position = pos;
	}
	~Vertex() {}
};

class Node
{
public:
	double  mass;           // In this project it will always be 1
	bool    isFixed;        // Use to pin the cloth
	Vec2    texCoord;       // Texture coord
	Vec3    normal;         // For smoothly shading
	Vec3	position;
	Vec3    velocity;
	Vec3    force;
	Vec3	acceleration;

public:
	Node(Vec3 pos = POSITION)
	{
		mass = MASS;
		isFixed = ISFIXED;
		position = pos;
		velocity.setZeroVec();
		force.setZeroVec();
		acceleration.setZeroVec();
	}
	~Node(void) {}

	void addForce(Vec3 force)
	{
		this->force += force;
	}

	/*
	 * Only non-fixed points get integrated
	 */
	void integrate(double timeStep)
	{
		// Verlet integration
		if (!isFixed)
		{
			// Newton's second law of motion
			acceleration = force / mass;
			velocity += acceleration * timeStep;
			position += velocity * timeStep;
		}
		force.setZeroVec();
	}
};