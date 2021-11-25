﻿#pragma once

#include <vector>

#include "Spring.h"
#include "Rigid.h"

enum Draw_Mode {
	DRAW_NODES,
	DRAW_LINES,
	DRAW_FACES
};

class Cloth
{
public:
	const int nodesDensity = 4;
	const static int iterationFreq = 10;
	// const double structuralCoef = 1000.0;
	// const double shearCoef = 50.0;
	// const double bendingCoef = 400.0;
	const float structuralCoef = 250.0;
	const float shearCoef = 12.5;
	const float bendingCoef = 100.0;

	static Draw_Mode drawMode;

	glm::vec3 clothPos;

	int width, height;
	int nodesPerRow, nodesPerCol;
	int clothID;

	std::vector<Node*> nodes;
	std::vector<Spring*> springs;
	std::vector<Node*> faces;
	std::vector<glm::vec2> pins;

	Cloth(glm::vec3 pos, glm::vec2 size, int ID)
	{
		clothPos = pos;
		width = size.x;
		height = size.y;
		clothID = ID;
		init();
	}

	~Cloth()
	{
		for (int i = 0; i < nodes.size(); i++)
		{
			delete nodes[i];
		}
		for (int i = 0; i < springs.size(); i++)
		{
			delete springs[i];
		}
		nodes.clear();
		springs.clear();
		faces.clear();
	}

	Node* getNode(int x, int y)
	{
		return nodes[y * nodesPerRow + x];
	}

	glm::vec3 computeFaceNormal(Node* n1, Node* n2, Node* n3)
	{
		return glm::cross(n2->position - n1->position, n3->position - n1->position);
	}

	static void modifyDrawMode(Draw_Mode mode)
	{
		drawMode = mode;
	}

	void pin(glm::vec2 index)  // Unpin cloth's (x, y) node
	{
		if (!(index.x < 0 || index.x >= nodesPerRow || index.y < 0 || index.y >= nodesPerCol)) 
		{
			getNode(index.x, index.y)->isFixed = true;
		}
	}

	void unPin(glm::vec2 index) // Unpin cloth's (x, y) node
	{
		if (!(index.x < 0 || index.x >= nodesPerRow || index.y < 0 || index.y >= nodesPerCol))
		{
			getNode(index.x, index.y)->isFixed = false;
		}
	}

	void init()
	{
		nodesPerRow = width * nodesDensity;
		nodesPerCol = height * nodesDensity;

		for (int i = 0; i < nodesPerRow; i += 1)
		{
			pins.push_back(glm::vec2(i, 0));
		}

		/** Add nodes **/
		printf("Init cloth with %d nodes\n", nodesPerRow * nodesPerCol);
		for (int i = 0; i < nodesPerRow; i++) {
			for (int j = 0; j < nodesPerCol; j++) {
				/** Create node by position **/
				Node* node = new Node(glm::vec3((float)j / nodesDensity, -((float)i / nodesDensity), 0));
				/** Set texture coordinates **/
				node->texCoord.x = (float)j / (nodesPerRow - 1);
				node->texCoord.y = (float)i / (1 - nodesPerCol);
				/** Add node to cloth **/
				nodes.push_back(node);

				printf("\t[%d, %d] (%f, %f, %f) - (%f, %f)\n", i, j, node->position.x, node->position.y, node->position.z, node->texCoord.x, node->texCoord.y);
			}
			std::cout << std::endl;
		}

		/** Add springs **/
		for (int i = 0; i < nodesPerRow; i++) {
			for (int j = 0; j < nodesPerCol; j++) {
				/** Structural **/
				if (i < nodesPerRow - 1) springs.push_back(new Spring(getNode(i, j), getNode(i + 1, j), structuralCoef));
				if (j < nodesPerCol - 1) springs.push_back(new Spring(getNode(i, j), getNode(i, j + 1), structuralCoef));
				/** Shear: 切应力, 由对角位置的两个质点组成弹簧 **/
				if (i < nodesPerRow - 1 && j < nodesPerCol - 1) {
					springs.push_back(new Spring(getNode(i, j), getNode(i + 1, j + 1), shearCoef));
					springs.push_back(new Spring(getNode(i + 1, j), getNode(i, j + 1), shearCoef));
				}
				/** Bending **/
				if (i < nodesPerRow - 2) springs.push_back(new Spring(getNode(i, j), getNode(i + 2, j), bendingCoef));
				if (j < nodesPerCol - 2) springs.push_back(new Spring(getNode(i, j), getNode(i, j + 2), bendingCoef));
			}
		}

		for (const glm::vec2& p : pins)
		{
			pin(p);
		}

		/** Triangle faces **/
		for (int i = 0; i < nodesPerRow - 1; i++) {
			for (int j = 0; j < nodesPerCol - 1; j++) {
				// Left upper triangle
				faces.push_back(getNode(i + 1, j));
				faces.push_back(getNode(i, j));
				faces.push_back(getNode(i, j + 1));
				// Right bottom triangle
				faces.push_back(getNode(i + 1, j + 1));
				faces.push_back(getNode(i + 1, j));
				faces.push_back(getNode(i, j + 1));
			}
		}
	}

	void computeNormal()
	{
		/** Reset nodes' normal **/
		glm::vec3 normal(0.0, 0.0, 0.0);
		for (int i = 0; i < nodes.size(); i++) {
			nodes[i]->normal = normal;
		}
		/** Compute normal of each face **/
		for (int i = 0; i < faces.size() / 3; i++) { // 3 nodes in each face
			Node* n1 = faces[3 * i + 0];
			Node* n2 = faces[3 * i + 1];
			Node* n3 = faces[3 * i + 2];

			// Face normal
			normal = computeFaceNormal(n1, n2, n3);
			// Add all face normal
			n1->normal += normal;
			n2->normal += normal;
			n3->normal += normal;
		}

		for (int i = 0; i < nodes.size(); i++) {
			nodes[i]->normal = glm::normalize(nodes[i]->normal);
		}
	}

	void addForce(glm::vec3 force)
	{
		for (int i = 0; i < nodes.size(); i++)
		{
			nodes[i]->addForce(force);
		}
	}

	void computeForce(float timeStep, glm::vec3 gravity)
	{
		/** Nodes **/
		for (int i = 0; i < nodes.size(); i++)
		{
			// 质点受重力的影响
			nodes[i]->addForce(gravity * nodes[i]->mass);
		}
		/** Springs **/
		for (int i = 0; i < springs.size(); i++)
		{
			// 质点是由弹簧连接
			// 每个 timestep 都要重新计算弹簧力
			springs[i]->computeInternalForce(timeStep);
		}
	}

	void integrate(float timeStep)
	{
		/** Node **/
		for (int i = 0; i < nodes.size(); i++)
		{
			nodes[i]->integrate(timeStep);
		}
	}

	glm::vec3 getWorldPos(Node* n)
	{
		return clothPos + n->position;
	}

	void setWorldPos(Node* n, glm::vec3 pos)
	{
		n->position = pos - clothPos;
	}

	void collisionResponse(Ground* ground)
	{
		for (int i = 0; i < nodes.size(); i++)
		{
			/** Ground collision **/
			if (getWorldPos(nodes[i]).y < ground->position.y) {
				nodes[i]->position.y = ground->position.y - clothPos.y + 0.01;
				nodes[i]->velocity = nodes[i]->velocity * ground->friction;
			}

			/** Ball collision **/
			// Vec3 distVec = getWorldPos(nodes[i]) - ball->center;
			// double distLen = distVec.length();
			// double safeDist = ball->radius * 1.05;
			// if (distLen < safeDist) {
			// 	distVec.normalize();
			// 	setWorldPos(nodes[i], distVec * safeDist + ball->center);
			// 	nodes[i]->velocity = nodes[i]->velocity * ball->friction;
			// }
		}
	}

	void move(glm::vec3 offset)
	{
		clothPos += offset;
	}

	glm::mat4 GetModelMatrix() const
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(clothPos.x, clothPos.y, clothPos.z));
		return model;
	}
};
