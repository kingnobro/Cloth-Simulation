#pragma once

#include <iostream>

#include "Cloth.h"
#include "Rigid.h"
#include "stb_image.h"
#include "Camera.h"
#include "Shader.h"

glm::vec3 lightPos(-5.0f, 7.0f, 6.0f);
glm::vec3 lightColor(0.7f, 0.7f, 1.0f);
Camera camera(glm::vec3(0.0f, 5.5f, 15.0f));

Draw_Mode Cloth::drawMode = DRAW_FACES;

struct ClothRender // Texture & Lighting
{
	const Cloth* cloth;
	int nodeCount; // Number of all nodes in faces

	glm::vec3* vboPos; // Position
	glm::vec2* vboTex; // Texture
	glm::vec3* vboNor; // Normal

	GLuint vaoID;
	GLuint vboIDs[3];
	GLuint texID;

	GLint aPtrPos;
	GLint aPtrTex;
	GLint aPtrNor;

	Shader shader;

	ClothRender(Cloth* cloth)
	{
		nodeCount = (int)(cloth->faces.size());
		if (nodeCount <= 0)
		{
			std::cout << "ERROR::ClothRender : No node exists." << std::endl;
			exit(-1);
		}

		this->cloth = cloth;

		vboPos = new glm::vec3[nodeCount];
		vboTex = new glm::vec2[nodeCount];
		vboNor = new glm::vec3[nodeCount];
		for (int i = 0; i < nodeCount; i++) 
		{
			Node* n = cloth->faces[i];
			vboPos[i] = n->position;
			vboTex[i] = n->texCoord; // Texture coord will only be set here
			vboNor[i] = n->normal;
		}

		/** Build shader **/
		shader = Shader("resources/Shaders/ClothVS.glsl", "resources/Shaders/ClothFS.glsl");
		std::cout << "Cloth Program ID: " << shader.ID << std::endl;

		// Generate ID of VAO and VBOs
		glGenVertexArrays(1, &vaoID);
		glGenBuffers(3, vboIDs);

		// Attribute pointers of VAO
		aPtrPos = 0;
		aPtrTex = 1;
		aPtrNor = 2;
		// Bind VAO
		glBindVertexArray(vaoID);

		// Position buffer
		glBindBuffer(GL_ARRAY_BUFFER, vboIDs[0]);
		glVertexAttribPointer(aPtrPos, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glBufferData(GL_ARRAY_BUFFER, nodeCount * sizeof(glm::vec3), vboPos, GL_DYNAMIC_DRAW);
		// Texture buffer
		glBindBuffer(GL_ARRAY_BUFFER, vboIDs[1]);
		glVertexAttribPointer(aPtrTex, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glBufferData(GL_ARRAY_BUFFER, nodeCount * sizeof(glm::vec2), vboTex, GL_DYNAMIC_DRAW);
		// Normal buffer
		glBindBuffer(GL_ARRAY_BUFFER, vboIDs[2]);
		glVertexAttribPointer(aPtrNor, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glBufferData(GL_ARRAY_BUFFER, nodeCount * sizeof(glm::vec3), vboNor, GL_DYNAMIC_DRAW);

		// Enable it's attribute pointers since they were set well
		glEnableVertexAttribArray(aPtrPos);
		glEnableVertexAttribArray(aPtrTex);
		glEnableVertexAttribArray(aPtrNor);

		/** Load texture **/
		// Assign texture ID and gengeration
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);
		// Set the texture wrapping parameters (for 2D tex: S, T)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// Set texture filtering parameters (Minify, Magnify)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		/** Load image and configure texture **/
		stbi_set_flip_vertically_on_load(true);
		int texW, texH, colorChannels; // After loading the image, stb_image will fill them
		unsigned char* data = stbi_load("resources/Textures/fiber2.jpg", &texW, &texH, &colorChannels, 0);
		if (data) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texW, texH, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			// Automatically generate all the required mipmaps for the currently bound texture.
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else {
			cout << "Failed to load texture" << endl;
		}
		// Always free image memory
		stbi_image_free(data);

		/** Set uniform **/
		shader.use(); // Active shader before set uniform
		// Set texture sampler
		shader.setInt("uniTex", 0);

		/** Set Matrix **/
		glm::mat4 uniModelMatrix = cloth->GetModelMatrix();
		shader.setMat4("uniProjMatrix", camera.GetProjectionMatrix());
		shader.setMat4("uniModelMatrix", uniModelMatrix);

		/** Set Light **/
		shader.setVec3("uniLightPos", lightPos);
		shader.setVec3("uniLightColor", lightColor);

		// Cleanup
		glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbined VBO
		glBindVertexArray(0); // Unbined VAO
	}

	void destroy()
	{
		delete[] vboPos;
		delete[] vboTex;
		delete[] vboNor;

		if (vaoID)
		{
			glDeleteVertexArrays(1, &vaoID);
			glDeleteBuffers(3, vboIDs);
			vaoID = 0;
		}
		if (shader.ID)
		{
			glDeleteProgram(shader.ID);
			shader.ID = 0;
		}
	}

	void flush()
	{
		// Update all the positions of nodes
		for (int i = 0; i < nodeCount; i++) 
		{ // Tex coordinate dose not change
			Node* n = cloth->faces[i];
			vboPos[i] = n->position;
			vboNor[i] = n->normal;
		}

		shader.use();

		glBindVertexArray(vaoID);

		glBindBuffer(GL_ARRAY_BUFFER, vboIDs[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, nodeCount * sizeof(glm::vec3), vboPos);
		glBindBuffer(GL_ARRAY_BUFFER, vboIDs[1]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, nodeCount * sizeof(glm::vec2), vboTex);
		glBindBuffer(GL_ARRAY_BUFFER, vboIDs[2]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, nodeCount * sizeof(glm::vec3), vboNor);

		/** Bind texture **/
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texID);

		/** Update Matrix **/
		glm::mat4 uniModelMatrix = cloth->GetModelMatrix();
		shader.setMat4("uniViewMatrix", camera.GetViewMatrix());
		shader.setMat4("uniModelMatrix", uniModelMatrix);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		/** Draw **/
		switch (Cloth::drawMode) 
		{
		case DRAW_NODES:
			glDrawArrays(GL_POINTS, 0, nodeCount);
			break;
		case DRAW_LINES:
			glDrawArrays(GL_LINES, 0, nodeCount);
			break;
		default:
			glDrawArrays(GL_TRIANGLES, 0, nodeCount);
			break;
		}

		// End flushing
		glDisable(GL_BLEND);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		glUseProgram(0);
	}
};

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

struct RigidRender // Single color & Lighting
{
	std::vector<Vertex*> faces;
	int vertexCount; // Number of nodes in faces

	glm::vec4 uniRigidColor;

	glm::vec3* vboPos; // Position
	glm::vec3* vboNor; // Normal

	GLuint vaoID;
	GLuint vboIDs[2];

	GLint aPtrPos;
	GLint aPtrNor;

	Shader shader;

	// Render any rigid body only with it's faces, color and modelVector
	void init(std::vector<Vertex*> f, glm::vec4 c, glm::vec3 modelVec)
	{
		faces = f;
		vertexCount = (int)(faces.size());
		if (vertexCount <= 0) {
			std::cout << "ERROR::RigidRender : No vertex exists." << std::endl;
			exit(-1);
		}

		uniRigidColor = c;

		vboPos = new glm::vec3[vertexCount];
		vboNor = new glm::vec3[vertexCount];
		for (int i = 0; i < vertexCount; i++) {
			Vertex* v = faces[i];
			vboPos[i] = glm::vec3(v->position.x, v->position.y, v->position.z);
			vboNor[i] = glm::vec3(v->normal.x, v->normal.y, v->normal.z);
		}

		/** Build shader **/
		shader = Shader("resources/Shaders/RigidVS.glsl", "resources/Shaders/RigidFS.glsl");
		std::cout << "Rigid Program ID: " << shader.ID << std::endl;

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
		glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(glm::vec3), vboPos, GL_DYNAMIC_DRAW);
		// Normal buffer
		glBindBuffer(GL_ARRAY_BUFFER, vboIDs[1]);
		glVertexAttribPointer(aPtrNor, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(glm::vec3), vboNor, GL_DYNAMIC_DRAW);

		// Enable it's attribute pointers since they were set well
		glEnableVertexAttribArray(aPtrPos);
		glEnableVertexAttribArray(aPtrNor);

		/** Set uniform **/
		shader.use(); // Active shader before set uniform
		// Set color
		shader.setVec4("uniRigidColor", uniRigidColor);

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
		shader.use();

		glBindVertexArray(vaoID);

		glBindBuffer(GL_ARRAY_BUFFER, vboIDs[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * sizeof(glm::vec3), vboPos);
		glBindBuffer(GL_ARRAY_BUFFER, vboIDs[1]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * sizeof(glm::vec3), vboNor);

		/** View Matrix : The camera **/
		shader.setMat4("uniViewMatrix", camera.GetViewMatrix());

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		/** Draw **/
		glDrawArrays(GL_TRIANGLES, 0, vertexCount);

		// End flushing
		glDisable(GL_BLEND);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		glUseProgram(0);
	}
};

class GroundRender
{
public:
	Ground* ground;
	RigidRender render;

	GroundRender(Ground* g)
	{
		ground = g;
		render.init(ground->faces, ground->color, ground->position);
	}

	void flush() 
	{ 
		render.flush(); 
	}
};

class BallRender
{
public:
	Ball* ball;
	RigidRender render;

	BallRender(Ball* b)
	{
		ball = b;
		render.init(ball->sphere->faces, ball->color, ball->center);
	}

	void flush() 
	{ 
		render.flush(); 
	}
};
