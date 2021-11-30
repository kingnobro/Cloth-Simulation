#pragma once

#include "../Display.h"

struct SpringRender
{
	std::vector<Spring*> springs;
	int springCount; // Number of nodes in springs

	glm::vec4 uniSpringColor;

	glm::vec3* vboPos; // Position
	glm::vec3* vboNor; // Normal

	GLuint vaoID;
	GLuint vboIDs[2];

	GLint aPtrPos;
	GLint aPtrNor;

	Shader shader;

	// Render any spring set, color and modelVector
	void init(std::vector<Spring*> s, glm::vec4 c, glm::vec3 modelVec)
	{
		springs = s;
		springCount = (int)(springs.size());
		if (springCount <= 0) {
			std::cout << "ERROR::SpringRender : No node exists." << std::endl;
			exit(-1);
		}

		uniSpringColor = c;

		vboPos = new glm::vec3[springCount * 2];
		vboNor = new glm::vec3[springCount * 2];
		for (int i = 0; i < springCount; i++) {
			Node* node1 = springs[i]->node1;
			Node* node2 = springs[i]->node2;
			vboPos[i * 2] = node1->position;
			vboPos[i * 2 + 1] = node2->position;
			vboNor[i * 2] = node1->normal;
			vboNor[i * 2 + 1] = node2->normal;
		}

		/** Build shader **/
		shader = Shader("resources/Shaders/SpringVS.glsl", "resources/Shaders/SpringFS.glsl");
		std::cout << "Spring Program ID: " << shader.ID << std::endl;

		// Generate ID of VAO and VBOs
		glGenVertexArrays(1, &vaoID);
		glGenBuffers(2, vboIDs);

		// Attribute pointers of VAO
		aPtrPos = 0;
		aPtrNor = 1;
		// Bind VAO
		glBindVertexArray(vaoID);

		// Position buffer
		glBindBuffer(GL_ARRAY_BUFFER, vboIDs[0]);
		glVertexAttribPointer(aPtrPos, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glBufferData(GL_ARRAY_BUFFER, springCount * 2 * sizeof(glm::vec3), vboPos, GL_DYNAMIC_DRAW);
		// Normal buffer
		glBindBuffer(GL_ARRAY_BUFFER, vboIDs[1]);
		glVertexAttribPointer(aPtrNor, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glBufferData(GL_ARRAY_BUFFER, springCount * 2 * sizeof(glm::vec3), vboNor, GL_DYNAMIC_DRAW);

		// Enable it's attribute pointers since they were set well
		glEnableVertexAttribArray(aPtrPos);
		glEnableVertexAttribArray(aPtrNor);

		/** Set uniform **/
		shader.use(); // Active shader before set uniform
		// Set color
		shader.setVec4("uniSpringColor", uniSpringColor);

		/** Projection matrix : The frustum that camera observes **/
		// Since projection matrix rarely changes, set it outside the rendering loop for only onec time
		shader.setMat4("uniProjMatrix", camera.GetProjectionMatrix());

		/** Model Matrix : Put rigid into the world **/
		glm::mat4 uniModelMatrix = glm::mat4(1.0f);
		uniModelMatrix = glm::translate(uniModelMatrix, modelVec);
		shader.setMat4("uniModelMatrix", uniModelMatrix);

		/** Light **/
		shader.setVec3("uniLightPos", lightPos);
		shader.setVec3("uniLightColor", lightColor);

		// Cleanup
		glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbined VBO
		glBindVertexArray(0); // Unbined VAO
	}

	void destroy()
	{
		delete[] vboPos;
		delete[] vboNor;

		if (vaoID)
		{
			glDeleteVertexArrays(1, &vaoID);
			glDeleteBuffers(2, vboIDs);
			vaoID = 0;
		}
		if (shader.ID)
		{
			glDeleteProgram(shader.ID);
			shader.ID = 0;
		}
	}

	void flush() // Rigid does not move, thus do not update vertexes' data
	{
		// Update all the positions of nodes
		for (int i = 0; i < springCount; i++) {
			Node* node1 = springs[i]->node1;
			Node* node2 = springs[i]->node2;
			vboPos[i * 2] = glm::vec3(node1->position.x, node1->position.y, node1->position.z);
			vboPos[i * 2 + 1] = glm::vec3(node2->position.x, node2->position.y, node2->position.z);
			vboNor[i * 2] = glm::vec3(node1->normal.x, node1->normal.y, node1->normal.z);
			vboNor[i * 2 + 1] = glm::vec3(node2->normal.x, node2->normal.y, node2->normal.z);
		}

		shader.use();

		glBindVertexArray(vaoID);

		glBindBuffer(GL_ARRAY_BUFFER, vboIDs[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, springCount * 2 * sizeof(glm::vec3), vboPos);
		glBindBuffer(GL_ARRAY_BUFFER, vboIDs[1]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, springCount * 2 * sizeof(glm::vec3), vboNor);

		/** View Matrix : The camera **/
		shader.setMat4("uniViewMatrix", camera.GetViewMatrix());

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		/** Draw **/
		glDrawArrays(GL_LINES, 0, springCount * 2);

		// End flushing
		glDisable(GL_BLEND);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		glUseProgram(0);
	}
};

struct ClothSpringRender
{
	Cloth* cloth;
	glm::vec4 defaultColor;
	SpringRender render;

	ClothSpringRender(Cloth* c)
	{
		cloth = c;
		defaultColor = glm::vec4(1.0f);
		render.init(cloth->springs, defaultColor, cloth->clothPos);
	}

	void flush() { render.flush(); }
};