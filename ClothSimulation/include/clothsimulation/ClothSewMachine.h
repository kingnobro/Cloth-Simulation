#pragma once

#include <glm/glm.hpp>
#include "Cloth.h"
#include "Shader.h"

class ClothSewMachine
{
public:
	Cloth* cloth1;
	Cloth* cloth2;
	Camera* camera;

	GLuint VAO;
	GLuint VBO;
	Shader shader;

	glm::vec3* vertices;
	unsigned int vertexNumber;

	ClothSewMachine(Camera* cam)
	{
		cloth1 = nullptr;
		cloth2 = nullptr;
		camera = cam;
		vertices = nullptr;
		vertexNumber = 0;
	}

	~ClothSewMachine()
	{
		delete[] vertices;

		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteProgram(shader.ID);
	}

	void initialization()
	{
		// todo: 任意边缝合时 vertexNumber 要改动
		vertexNumber = cloth1->nodesPerRow * 2;

		// generate vertices
		vertices = new glm::vec3[vertexNumber];
		for (int i = 0; i < vertexNumber / 2; i++) {
			vertices[i * 2] = cloth1->getWorldPos(cloth1->getNode(i, 0));
			vertices[i * 2 + 1] = cloth2->getWorldPos(cloth2->getNode(i, 0));
		}

		shader = Shader("resources/Shaders/LineVS.glsl", "resources/Shaders/LineFS.glsl");
		std::cout << "Sew Program ID: " << shader.ID << std::endl;

		// generate ID of VAO and VBO
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);

		// Bind VAO
		glBindVertexArray(VAO);

		// position buffer
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
		glBufferData(GL_ARRAY_BUFFER, vertexNumber * sizeof(glm::vec3), vertices, GL_DYNAMIC_DRAW);

		// enalbe attribute pointers
		glEnableVertexAttribArray(0);

		// set Uniforms
		// 顶点坐标是世界坐标, 所以不需要传入 model 矩阵
		shader.use();
		shader.setMat4("view", camera->GetViewMatrix());
		shader.setMat4("projection", camera->GetProjectionMatrix());

		// Cleanup
		glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbined VBO
		glBindVertexArray(0);		      // Unbined VAO
	}

	/*
	 * 缝合衣片
	 * 不能直接修改 Cloth 的 clothPos, 因为修改 closhPos 会导致 modelMatrix 改变
	 * 这样的效果是, 下一个渲染循环中, 衣片瞬间移动
	 * 所以需要直接修改 Node 的局部坐标, 这样在下一个渲染循环中就会重新计算弹簧受力和质点位移, 产生物理效果
	 * 
	 * Cloth 只能缝合一次, 意味着 Node 的局部坐标只会修改一次, 不会出现坐标错乱的问题
	 */
	void SewCloths()
	{
		if (cloth1 == nullptr || cloth2 == nullptr) {
			return;
		}
		glm::vec3 delta = glm::abs(cloth1->clothPos - cloth2->clothPos) / (float)2;

		// 修改 Node 的局部坐标
		for (int i = 0; i < cloth1->nodesPerRow; i++) {
			Node* n1 = cloth1->getNode(i, 0);
			Node* n2 = cloth2->getNode(i, 0);
		
			if (cloth1->getWorldPos(n1).x > cloth2->getWorldPos(n2).x) {
				n1->position.x -= delta.x;
				n2->position.x += delta.x;
			} else {
				n1->position.x += delta.x;
				n2->position.x -= delta.x;
			}
		
			if (cloth1->getWorldPos(n1).y > cloth2->getWorldPos(n2).y) {
				n1->position.y -= delta.y;
				n2->position.y += delta.y;
			}
			else {
				n1->position.y += delta.y;
				n2->position.y -= delta.y;
			}
		
			if (cloth1->getWorldPos(n1).z > cloth2->getWorldPos(n2).z) {
				n1->position.z -= delta.z;
				n2->position.z += delta.z;
			}
			else {
				n1->position.z += delta.z;
				n2->position.z -= delta.z;
			}
		}
	}

	/*
	 * 绘制衣片的缝合线 
	 */
	void drawSewingLine(const glm::mat4 &view)
	{
		if (cloth1 == nullptr || cloth2 == nullptr) {
			return;
		}
		// 衣片的位置会更新, 所以线的位置也要更新
		for (int i = 0; i < vertexNumber / 2; i++) {
			vertices[i * 2] = cloth1->getWorldPos(cloth1->getNode(i, 0));
			vertices[i * 2 + 1] = cloth2->getWorldPos(cloth2->getNode(i, 0));
		}

		shader.use();
		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertexNumber * sizeof(glm::vec3), vertices);

		shader.setMat4("view", view);
		
		// draw
		glDrawArrays(GL_LINES, 0, vertexNumber);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		glUseProgram(0);
	}

	// todo: 实现更复杂的交互逻辑
	void setCandidateCloth(Cloth* cloth)
	{
		if (cloth1 == nullptr) {
			cloth1 = cloth;
		}
		else if (cloth1 != cloth && cloth == nullptr)
		{
			cloth2 = cloth;
			initialization();
		}
	}
};