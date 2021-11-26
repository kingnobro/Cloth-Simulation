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
		// todo: ����߷��ʱ vertexNumber Ҫ�Ķ�
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
		// ������������������, ���Բ���Ҫ���� model ����
		shader.use();
		shader.setMat4("view", camera->GetViewMatrix());
		shader.setMat4("projection", camera->GetProjectionMatrix());

		// Cleanup
		glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbined VBO
		glBindVertexArray(0);		      // Unbined VAO
	}

	/*
	 * �����Ƭ
	 * ����ֱ���޸� Cloth �� clothPos, ��Ϊ�޸� closhPos �ᵼ�� modelMatrix �ı�
	 * ������Ч����, ��һ����Ⱦѭ����, ��Ƭ˲���ƶ�
	 * ������Ҫֱ���޸� Node �ľֲ�����, ��������һ����Ⱦѭ���оͻ����¼��㵯���������ʵ�λ��, ��������Ч��
	 * 
	 * Cloth ֻ�ܷ��һ��, ��ζ�� Node �ľֲ�����ֻ���޸�һ��, �������������ҵ�����
	 */
	void SewCloths()
	{
		if (cloth1 == nullptr || cloth2 == nullptr) {
			return;
		}
		glm::vec3 delta = glm::abs(cloth1->clothPos - cloth2->clothPos) / (float)2;

		// �޸� Node �ľֲ�����
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
	 * ������Ƭ�ķ���� 
	 */
	void drawSewingLine(const glm::mat4 &view)
	{
		if (cloth1 == nullptr || cloth2 == nullptr) {
			return;
		}
		// ��Ƭ��λ�û����, �����ߵ�λ��ҲҪ����
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

	// todo: ʵ�ָ����ӵĽ����߼�
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