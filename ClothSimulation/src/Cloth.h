#pragma once

#include <vector>

#include "Spring.h"
#include "ModelRender.h"

extern const int iterationFreq;
extern glm::vec3 gravity;

// Default Cloth Values
const int NODE_DENSITY = 4;
const float STRUCTURAL_COEF = 400.0;
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
	bool sewed;

	std::vector<Node*> nodes;
	std::vector<Node*> faces;
	std::vector<Node*> sewNode;	// 即将被缝合的顶点
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
	void update(double timeStep, ModelRender& modelRender)
	{
		for (int i = 0; i < iterationFreq; i++)
		{
			computeForce(timeStep, gravity);
			integrate(timeStep);
			for (Node* node : nodes)
			{
				if (modelRender.collideWithModel(getWorldPos(node))) {
					// std::cout << "collided\n";
					modelRender.collisionResponse(node, clothPos);
				}
			}
		}
		computeFaceNormal();
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

	int GetClothID() const
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

	/*
	 * 重置局部坐标	
	 */
	void reset()
	{
		Node* node = nullptr;
		for (int y = 0; y < nodesPerCol; y++) {
			for (int x = 0; x < nodesPerRow; x++) {
				float pos_x = (float)x / nodesDensity;
				float pos_y = -((float)y / nodesDensity);
				float pos_z = 0;
				node = nodes[y * nodesPerRow + x];
				node->lastPosition = node->position = glm::vec3(pos_x, pos_y, pos_z);
			}
		}
		// 恢复到未缝合的状态
		sewed = false;
	}

private:
	void init()
	{
		nodesPerRow = width * nodesDensity;
		nodesPerCol = height * nodesDensity;

		/** Add Nodes **/
		printf("Init cloth with %d nodes\n", nodesPerRow * nodesPerCol);
		for (int y = 0; y < nodesPerCol; y++) {
			for (int x = 0; x < nodesPerRow; x++) {
				float pos_x = (float)x / nodesDensity;
				float pos_y = -((float)y / nodesDensity);   // 衣物原点在左上角, 所以衣服上的点 y 值是负的
				float pos_z = 0;
				float tex_x = (float)x / (nodesPerRow - 1);
				float tex_y = (float)y / (1 - nodesPerCol);
				Node* node = new Node(glm::vec3(pos_x, pos_y, pos_z));
				node->texCoord = glm::vec2(tex_x, tex_y);
				nodes.push_back(node);
			}
		}

		/** Add springs **/
		for (int x = 0; x < nodesPerRow; x++) {
			for (int y = 0; y < nodesPerCol; y++) {
				/** Structural **/
				if (x < nodesPerRow - 1) springs.push_back(new Spring(getNode(x, y), getNode(x + 1, y), structuralCoef));
				if (y < nodesPerCol - 1) springs.push_back(new Spring(getNode(x, y), getNode(x, y + 1), structuralCoef));
				/** Shear **/
				if (x < nodesPerRow - 1 && y < nodesPerCol - 1) {
					springs.push_back(new Spring(getNode(x, y), getNode(x + 1, y + 1), shearCoef));
					springs.push_back(new Spring(getNode(x + 1, y), getNode(x, y + 1), shearCoef));
				}
				/** Bending **/
				if (x < nodesPerRow - 2) springs.push_back(new Spring(getNode(x, y), getNode(x + 2, y), bendingCoef));
				if (y < nodesPerCol - 2) springs.push_back(new Spring(getNode(x, y), getNode(x, y + 2), bendingCoef));
			}
		}

		/** Triangle faces **/
		for (int x = 0; x < nodesPerRow - 1; x++) {
			for (int y = 0; y < nodesPerCol - 1; y++) {
				// Left upper triangle
				faces.push_back(getNode(x + 1, y));
				faces.push_back(getNode(x, y));
				faces.push_back(getNode(x, y + 1));
				// Right bottom triangle
				faces.push_back(getNode(x + 1, y + 1));
				faces.push_back(getNode(x + 1, y));
				faces.push_back(getNode(x, y + 1));
			}
		}

		pins.push_back(glm::vec2(0, 0));
		pins.push_back(glm::vec2(1, 0));
		pins.push_back(glm::vec2(2, 0));
		pins.push_back(glm::vec2(nodesPerRow - 1, 0));
		pins.push_back(glm::vec2(nodesPerRow - 2, 0));
		pins.push_back(glm::vec2(nodesPerRow - 3, 0));
		for (int i = 8; i < nodesPerCol; i++) {
			pins.push_back(glm::vec2(0, i));
			pins.push_back(glm::vec2(nodesPerRow - 1, i));
		}
		// pins.push_back(glm::vec2(0, nodesPerCol - 1));
		// pins.push_back(glm::vec2(nodesPerRow - 1, nodesPerCol - 1));
		for (const glm::vec2& p : pins) {
			pin(p);
		}

		// 添加待缝合的点
		sewNode.push_back(getNode(0, 0));
		sewNode.push_back(getNode(1, 0));
		sewNode.push_back(getNode(2, 0));
		sewNode.push_back(getNode(nodesPerRow - 1, 0));
		sewNode.push_back(getNode(nodesPerRow - 2, 0));
		sewNode.push_back(getNode(nodesPerRow - 3, 0));
		for (int i = 8; i < nodesPerCol; i++) {
			sewNode.push_back(getNode(0, i));
			sewNode.push_back(getNode(nodesPerRow - 1, i));
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
		if (index.x >=0 && index.x < nodesPerRow && index.y >= 0 && index.y < nodesPerCol) {
			getNode(index.x, index.y)->isFixed = true;
		}
	}

	void unPin(glm::vec2 index) // Unpin cloth's (x, y) node
	{
		if (index.x >= 0 && index.x < nodesPerRow && index.y >= 0 && index.y < nodesPerCol) {
			getNode(index.x, index.y)->isFixed = false;
		}
	}

	void computeFaceNormal()
	{
		/** Reset nodes' normal **/
		glm::vec3 normal(0.0f);
		for (Node* node : nodes) {
			node->normal = normal;
		}
		/** Compute normal of each face **/
		Node* n1;
		Node* n2;
		Node* n3;
		for (size_t i = 0; i < faces.size() / 3; i++) { // 3 nodes in each face
			n1 = faces[3 * i];
			n2 = faces[3 * i + 1];
			n3 = faces[3 * i + 2];

			// Face normal
			normal = glm::cross(n2->position - n1->position, n3->position - n1->position);
			// Add all face normal
			n1->normal += normal;
			n2->normal += normal;
			n3->normal += normal;
		}

		for (Node* node : nodes) {
			node->normal = glm::normalize(node->normal);
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
	std::vector<glm::vec2> pins;
};
