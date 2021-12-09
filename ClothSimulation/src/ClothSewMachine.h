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

	vector<glm::vec3> vertices;    // 衣片上的顶点
	bool resetable;	    // 经过 reset 处理后, VAO VBO 会被删除
						// 多次删除会报错, 所以用一个 bool 变量记录是否处于 reset 状态

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

		// 设置待缝合的顶点, 用于绘制缝合线
		setSewNode();

		// 绘制缝合线的 Shader
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
		// 布料的顶点坐标是世界坐标, 所以不需要传入 model 矩阵
		shader.use();
		shader.setMat4("view", camera->GetViewMatrix());
		shader.setMat4("projection", camera->GetPerspectiveProjectionMatrix());

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
	 * Cloth 只能缝合一次, 意味着 Node 的局部坐标只会修改一次; 缝合多次可能会出现局部坐标错乱的问题
	 */
	void SewCloths()
	{
		if (cloth1 == nullptr || cloth2 == nullptr || cloth1->sewed || cloth2->sewed) {
			return;
		}
		const vector<Node*>& sewNode1 = cloth1->sewNode;
		const vector<Node*>& sewNode2 = cloth2->sewNode;
		assert(sewNode1.size() == sewNode2.size());

		// 修改 Node 的局部坐标
		for (size_t i = 0, sz = sewNode1.size(); i < sz; i++) {
			Node* n1 = sewNode1[i];
			Node* n2 = sewNode2[i];
			glm::vec3 worldPos1 = cloth1->getWorldPos(n1);
			glm::vec3 worldPos2 = cloth2->getWorldPos(n2);
			glm::vec3 delta = glm::abs(cloth1->clothPos - cloth2->clothPos) / 2.0f;

			// 将两个质点朝着它们的中点移动
			// 为了简化表达式, 将 bool 转换为 int, 用于决定是 + 还是 -
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
	 * 绘制衣片的缝合线
	 */
	void drawSewingLine(const glm::mat4& view)
	{
		if (cloth1 == nullptr || cloth2 == nullptr || cloth1->sewed || cloth2->sewed) {
			return;
		}
		// 衣片的位置会更新, 所以线的位置也要更新
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
		// 重置衣片的局部坐标
		if (cloth1) cloth1->reset();
		if (cloth2) cloth2->reset();
		cloth1 = cloth2 = nullptr;

		// VAO VBO 只能删除一次
		if (resetable)
		{
			glDeleteVertexArrays(1, &VAO);
			glDeleteBuffers(1, &VBO);
			glDeleteProgram(shader.ID);
		}
		resetable = false;
	}


private:
	/* 每个衣片有一个待缝合的顶点数组
	 * 两个衣片的待缝合数组中的顶点需要一一对应
	 * 衣片的顶点(世界坐标)存放在 vertices 中, 并且 vertices[i] 和 vertices[i+1] 即将缝合 (i%2=0)
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