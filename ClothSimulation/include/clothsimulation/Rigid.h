#pragma once

#include <math.h>

#include <cmath>
#include <vector>
#include <corecrt_math_defines.h>

#include "Point.h"

struct Ground
{
	glm::vec3 position;
	int width, height;
	glm::vec4 color;
	const float friction = (float) 0.9;

	std::vector<Vertex*> vertexes;
	std::vector<Vertex*> faces;

	Ground(glm::vec3 position, glm::vec2 size, glm::vec4 color) {
		this->position = position;
		this->color = color;
		width = (int)size.x;
		height = (int)size.y;

		init();
	}
	~Ground()
	{
		for (int i = 0; i < vertexes.size(); i++) 
		{ 
			delete vertexes[i];
		}
		vertexes.clear();
		faces.clear();
	}

	void init()
	{
		vertexes.push_back(new Vertex(glm::vec3(0.0f)));
		vertexes.push_back(new Vertex(glm::vec3(width, 0.0, 0.0)));
		vertexes.push_back(new Vertex(glm::vec3(0.0, 0.0, -height)));
		vertexes.push_back(new Vertex(glm::vec3(width, 0.0, -height)));

		for (int i = 0; i < vertexes.size(); i++) {
			vertexes[i]->normal = glm::vec3(0.0, 1.0, 0.0); // It's not neccessery to normalize here

			// Debug info
			printf("Ground[%d]: (%f, %f, %f) - (%f, %f, %f)\n", i, vertexes[i]->position.x, vertexes[i]->position.y, vertexes[i]->position.z, vertexes[i]->normal.x, vertexes[i]->normal.y, vertexes[i]->normal.z);
		}

		faces.push_back(vertexes[0]);
		faces.push_back(vertexes[1]);
		faces.push_back(vertexes[2]);
		faces.push_back(vertexes[1]);
		faces.push_back(vertexes[2]);
		faces.push_back(vertexes[3]);
	}
};

class Sphere
{
public:
	const int meridianNum = 24;
	const int parallelNum = 250;

	int radius;

	std::vector<Vertex*> vertexes;
	std::vector<Vertex*> faces;

	Sphere(int r)
	{
		radius = r;
		init();
	}
	~Sphere()
	{
		for (int i = 0; i < vertexes.size(); i++) 
		{ 
			delete vertexes[i];
		}
		vertexes.clear();
		faces.clear();
	}

	Vertex* getTop()
	{ 
		return vertexes[0];
	}

	Vertex* getVertex(int x, int y)
	{
		if (x < 0 || x >= parallelNum || y < 0 || y >= meridianNum) 
		{
			printf("Vertex Index Out of Range.\n");
			exit(-1);
		}
		else
		{
			return vertexes[1 + x * meridianNum + y];
		}
	}

	Vertex* getBottom() 
	{ 
		return vertexes[vertexes.size() - 1]; 
	}

	glm::vec3 computeFaceNormal(Vertex* v1, Vertex* v2, Vertex* v3)
	{
		return glm::cross(v2->position - v1->position, v3->position - v1->position);
	}

	void computeSphereNormal()
	{
		glm::vec3 normal(0.0f);
		for (int i = 0; i < vertexes.size(); i++) {
			vertexes[i]->normal = normal;
		}

		// The normal of all faces of the first and last cycle should be calculated specially!
		for (int i = 0; i < faces.size() / 3; i++) {
			Vertex* v1 = faces[i * 3 + 0];
			Vertex* v2 = faces[i * 3 + 1];
			Vertex* v3 = faces[i * 3 + 2];

			normal = computeFaceNormal(v1, v3, v2);
			v1->normal += normal;
			v2->normal += normal;
			v3->normal += normal;
		}

		for (int i = 0; i < vertexes.size(); i++)
		{
			vertexes[i]->normal = glm::normalize(vertexes[i]->normal);
		}
	}

	void init() // Initialize vertices coord and slice faces
	{
		/** Compute vertex position **/
		float cycleInterval = radius * 2.0 / (parallelNum + 1);
		float radianInterval = 2.0 * M_PI / meridianNum;


		glm::vec3 pos(0.0, radius, 0.0);
		vertexes.push_back(new Vertex(pos)); // Top vertex

		for (int i = 0; i < parallelNum; i++) {
			pos.y -= cycleInterval;
			for (int j = 0; j < meridianNum; j++) {
				float xzLen = radius * sqrt(1.0 - pow(pos.y / radius, 2));
				float xRadian = j * radianInterval;  // The length of projection line on X-Z pane

				pos.x = xzLen * sin(xRadian);
				pos.z = xzLen * cos(xRadian);
				vertexes.push_back(new Vertex(pos));
			}
		}
		pos = glm::vec3(0.0, -radius, 0.0);
		vertexes.push_back(new Vertex(pos)); // Bottom vertex

		/** Slice faces **/
		// Top cycle
		for (int i = 0; i < meridianNum; i++) {
			faces.push_back(getVertex(0, i));                                   //   *   //
			faces.push_back(getTop());                                          //  / \  //
			faces.push_back(getVertex(0, (i + 1) % meridianNum));               // *---* //
		}
		// Middle cycles
		for (int i = 0; i < parallelNum - 1; i++) {
			for (int j = 0; j < meridianNum; j++) {
				faces.push_back(getVertex(i, j));                               //  *--* //
				faces.push_back(getVertex(i, (j + 1) % meridianNum));           //  | /  //
				faces.push_back(getVertex(i + 1, j));                           //  *    //

				faces.push_back(getVertex(i + 1, (j + 1) % meridianNum));       //     * //
				faces.push_back(getVertex(i + 1, j));                           //   / | //
				faces.push_back(getVertex(i, (j + 1) % meridianNum));           //  *--* //
			}
		}
		// Bottom cycle
		for (int i = 0; i < meridianNum; i++) {
			faces.push_back(getBottom());										  // *---* //
			faces.push_back(getVertex(parallelNum - 1, i));                       //  \ /  //
			faces.push_back(getVertex(parallelNum - 1, (i + 1) % meridianNum));   //   *   //
		}

		/** Set normals **/
		computeSphereNormal();
	}
};

class Ball
{
public:
	glm::vec3 center;
	int radius;
	glm::vec4 color;
	const float friction = 0.8;

	Sphere* sphere;

	Ball(glm::vec3 center, int radius, glm::vec4 color)
	{
		this->center = center;
		this->radius = radius;
		this->color = color;

		sphere = new Sphere(radius);
	}
	~Ball() {}
};
