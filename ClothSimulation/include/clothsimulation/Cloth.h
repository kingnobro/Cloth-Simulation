#pragma once

#include <vector>
#include <omp.h>

#include "Spring.h"
#include "Rigid.h"
#include "ModelRender.h"

extern const int iterationFreq;
extern glm::vec3 gravity;

// Default Cloth Values
const int NODE_DENSITY = 4;
const float STRUCTURAL_COEF = 500.0;
const float SHEAR_COEF = 30.0;
const float BENDING_COEF = 3.0;

enum Draw_Mode
{
	DRAW_NODES,
	DRAW_LINES,
	DRAW_FACES
};

class Cloth
{
public:
	static Draw_Mode drawMode;

	const int nodesDensity = NODE_DENSITY;
	const float structuralCoef = STRUCTURAL_COEF;
	const float shearCoef = SHEAR_COEF;
	const float bendingCoef = BENDING_COEF;
	int width;
	int height;
	int nodesPerRow;
	int nodesPerCol;
	glm::vec3 clothPos;

	std::vector<Node*> nodes;
	std::vector<Node*> faces;
	std::vector<Spring*> springs;

	Cloth(glm::vec3 pos, glm::vec2 size, int ID)
	{
		clothPos = pos;
		width = size.x;
		height = size.y;
		clothID = ID;
		sewed = false;

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

	static void modifyDrawMode(Draw_Mode mode)
	{
		drawMode = mode;
	}

	/*
	 * update force, movement, collision and normals in every render loop
	 */
	void update(double timeStep, ModelRender &modelRender)
	{
		for (int i = 0; i < iterationFreq; i++)
		{
			computeForce(timeStep, gravity);
			integrate(timeStep);
			for (Node* node : nodes)
			{
				if (modelRender.collideWithModel(getWorldPos(node))) {
					modelRender.collisionResponse(node, clothPos);
				}
			}
		}
		computeNormal();
	}

	glm::vec3 getWorldPos(Node* n)
	{
		return clothPos + n->position;
	}

	void setWorldPos(Node* n, glm::vec3 pos)
	{
		n->position = pos - clothPos;
	}

	void moveCloth(glm::vec3 offset)
	{
		clothPos += offset;
	}

	glm::mat4 GetModelMatrix() const
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(clothPos.x, clothPos.y, clothPos.z));
		return model;
	}

	int GetClothID()
	{
		return clothID;
	}

	Node* getNode(int x, int y)
	{
		if (x >= 0 && x < nodesPerRow && y >= 0 && y < nodesPerCol) {
			return nodes[y * nodesPerRow + x];
		}
		return nullptr;
	}

	void reset()
	{
		/** 重置局部坐标 **/
		for (int i = 0; i < nodesPerRow; i++) {
			for (int j = 0; j < nodesPerCol; j++) {
				float pos_x = (float)j / nodesDensity;
				float pos_y = -((float)i / nodesDensity);
				float pos_z = 0;
				nodes[i * nodesPerRow + j]->position = glm::vec3(pos_x, pos_y, pos_z);
				nodes[i * nodesPerRow + j]->lastPosition = glm::vec3(pos_x, pos_y, pos_z);
			}
		}
	}

private:
	void init()
	{
		nodesPerRow = width * nodesDensity;
		nodesPerCol = height * nodesDensity;

		for (int i = 0; i < 7; i += 1)
		{
			pins.push_back(glm::vec2(i, 0));
			pins.push_back(glm::vec2(nodesPerRow - i, 0));
		}

		/** Add Nodes **/
		printf("Init cloth with %d nodes\n", nodesPerRow * nodesPerCol);
		for (int i = 0; i < nodesPerRow; i++) {
			for (int j = 0; j < nodesPerCol; j++) {
				/** Create node by position **/
				float pos_x = (float)j / nodesDensity;
				float pos_y = -((float)i / nodesDensity);
				float pos_z = 0;
				float tex_x = (float)j / (nodesPerRow - 1);
				float tex_y = (float)i / (1 - nodesPerCol);
				Node* node = new Node(glm::vec3(pos_x, pos_y, pos_z));
				node->texCoord = glm::vec2(tex_x, tex_y);
				nodes.push_back(node);

				// printf("\t[%d, %d] (%f, %f, %f) - (%f, %f)\n", i, j, pos_x, pos_y, pos_z, tex_x, tex_y);
			}
			// std::cout << std::endl;
		}

		/** Add springs **/
		for (int i = 0; i < nodesPerRow; i++) {
			for (int j = 0; j < nodesPerCol; j++) {
				/** Structural **/
				if (i < nodesPerRow - 1) springs.push_back(new Spring(getNode(i, j), getNode(i + 1, j), structuralCoef));
				if (j < nodesPerCol - 1) springs.push_back(new Spring(getNode(i, j), getNode(i, j + 1), structuralCoef));
				/** Shear **/
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

	void computeForce(float timeStep, glm::vec3 gravity)
	{
		for (Node* node : nodes) {
			node->addForce(gravity * node->mass);
		}
		for (Spring* spring : springs) {
			spring->computeInternalForce(timeStep);
		}
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

	glm::vec3 computeFaceNormal(Node* n1, Node* n2, Node* n3)
	{
		return glm::cross(n2->position - n1->position, n3->position - n1->position);
	}

	void computeNormal()
	{
		/** Reset nodes' normal **/
		glm::vec3 normal(0.0f);
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
		for (Node* node : nodes) {
			node->addForce(force);
		}
	}

	void integrate(float timeStep)
	{
		for (Node* node : nodes) {
			node->integrate(timeStep);
		}
	}

	int clothID;
	bool sewed;

	std::vector<glm::vec2> pins;
};
