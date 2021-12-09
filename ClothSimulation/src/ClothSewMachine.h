#pragma once

#include <glm/glm.hpp>
#include <assert.h>
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

	vector<glm::vec3> vertices;    // ��Ƭ�ϵĶ���
	bool resetable;	    // ���� reset �����, VAO VBO �ᱻɾ��
						// ���ɾ���ᱨ��, ������һ�� bool ������¼�Ƿ��� reset ״̬

	ClothSewMachine(Camera* cam)
	{
		cloth1 = cloth2 = nullptr;
		camera = cam;
		resetable = false;
	}

	~ClothSewMachine()
	{
		if (resetable)
		{
			glDeleteVertexArrays(1, &VAO);
			glDeleteBuffers(1, &VBO);
			glDeleteProgram(shader.ID);
		}
	}

	void initialization()
	{
		resetable = true;

		// ���ô���ϵĶ���, ���ڻ��Ʒ����
		setSewNode();

		// ���Ʒ���ߵ� Shader
		shader = Shader("src/shaders/LineVS.glsl", "src/shaders/LineFS.glsl");
		std::cout << "Sew Program ID: " << shader.ID << std::endl;

		// generate ID of VAO and VBO
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);

		// Bind VAO
		glBindVertexArray(VAO);

		// position buffer
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_DYNAMIC_DRAW);

		// enalbe attribute pointers
		glEnableVertexAttribArray(0);

		// set Uniforms
		// ���ϵĶ�����������������, ���Բ���Ҫ���� model ����
		shader.use();
		shader.setMat4("view", camera->GetViewMatrix());
		shader.setMat4("projection", camera->GetPerspectiveProjectionMatrix());

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
	 * Cloth ֻ�ܷ��һ��, ��ζ�� Node �ľֲ�����ֻ���޸�һ��; ��϶�ο��ܻ���־ֲ�������ҵ�����
	 */
	void SewCloths()
	{
		if (cloth1 == nullptr || cloth2 == nullptr || cloth1->sewed || cloth2->sewed) {
			return;
		}
		const vector<Node*>& sewNode1 = cloth1->sewNode;
		const vector<Node*>& sewNode2 = cloth2->sewNode;
		assert(sewNode1.size() == sewNode2.size());

		// �޸� Node �ľֲ�����
		for (size_t i = 0, sz = sewNode1.size(); i < sz; i++) {
			Node* n1 = sewNode1[i];
			Node* n2 = sewNode2[i];
			glm::vec3 worldPos1 = cloth1->getWorldPos(n1);
			glm::vec3 worldPos2 = cloth2->getWorldPos(n2);
			glm::vec3 delta = glm::abs(cloth1->clothPos - cloth2->clothPos) / 2.0f;

			// �������ʵ㳯�����ǵ��е��ƶ�
			// Ϊ�˼򻯱��ʽ, �� bool ת��Ϊ int, ���ھ����� + ���� -
			n1->position.x += (1 - 2 * (worldPos1.x > worldPos2.x)) * delta.x;
			n2->position.x += (-1 + 2 * (worldPos1.x > worldPos2.x)) * delta.x;
			n1->position.y += (1 - 2 * (worldPos1.y > worldPos2.y)) * delta.y;
			n2->position.y += (-1 + 2 * (worldPos1.y > worldPos2.y)) * delta.y;
			n1->position.z += (1 - 2 * (worldPos1.z > worldPos2.z)) * delta.z;
			n2->position.z += (-1 + 2 * (worldPos1.z > worldPos2.z)) * delta.z;
		}

		cloth1->sewed = cloth2->sewed = true;
	}

	/*
	 * ������Ƭ�ķ����
	 */
	void drawSewingLine(const glm::mat4& view)
	{
		if (cloth1 == nullptr || cloth2 == nullptr || cloth1->sewed || cloth2->sewed) {
			return;
		}
		// ��Ƭ��λ�û����, �����ߵ�λ��ҲҪ����
		setSewNode();

		shader.use();
		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(glm::vec3), vertices.data());

		shader.setMat4("view", view);

		// draw
		glDrawArrays(GL_LINES, 0, vertices.size());

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		glUseProgram(0);
	}

	void setCandidateCloth(Cloth* cloth)
	{
		if (cloth == nullptr)
		{
			return;
		}
		if (cloth1 == nullptr)
		{
			cloth1 = cloth;
		}
		else if (cloth1 != cloth && cloth2 == nullptr)
		{
			cloth2 = cloth;
			initialization();
		}
	}

	void reset()
	{
		// ������Ƭ�ľֲ�����
		if (cloth1) cloth1->reset();
		if (cloth2) cloth2->reset();
		cloth1 = cloth2 = nullptr;

		// VAO VBO ֻ��ɾ��һ��
		if (resetable)
		{
			glDeleteVertexArrays(1, &VAO);
			glDeleteBuffers(1, &VBO);
			glDeleteProgram(shader.ID);
		}
		resetable = false;
	}


private:
	/* ÿ����Ƭ��һ������ϵĶ�������
	 * ������Ƭ�Ĵ���������еĶ�����Ҫһһ��Ӧ
	 * ��Ƭ�Ķ���(��������)����� vertices ��, ���� vertices[i] �� vertices[i+1] ������� (i%2=0)
	 */ 
	void setSewNode() {
		vertices.clear();
		const vector<Node*>& sewNode1 = cloth1->sewNode;
		const vector<Node*>& sewNode2 = cloth2->sewNode;
		assert(sewNode1.size() == sewNode2.size());

		for (size_t i = 0, sz = sewNode1.size(); i < sz; i++) {
			vertices.push_back(cloth1->getWorldPos(sewNode1[i]));
			vertices.push_back(cloth2->getWorldPos(sewNode2[i]));
		}
	}
};