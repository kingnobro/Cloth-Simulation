#pragma once

#include "Model.h"

typedef unsigned int uint;

class ModelRender
{
public:
	ModelRender(Model* model)
	{
		this->model = model;
		this->init();
	}

	~ModelRender()
	{
		delete[]frontDepthMap;
		delete[]frontNormalMap;
		delete[]backDepthMap;
		delete[]backNormalMap;
		// detele shaders
		if (runtimeShader.ID) {
			glDeleteProgram(runtimeShader.ID);
			runtimeShader.ID = 0;
		}
		if (offlineShader.ID) {
			glDeleteProgram(offlineShader.ID);
			offlineShader.ID = 0;
		}
	}

	void offScreenRender(Camera* frontCamera, Camera* backCamera, GLFWwindow* window)
	{
		// screen size
		// in the paper, scr_width = scr_height = mapsize = 512
		glfwGetWindowSize(window, &scr_width, &scr_height);
		const size_t resolution = scr_width * scr_height;
		frontDepthMap = new float[resolution];
		frontNormalMap = new float[resolution * 3];
		backDepthMap = new float[resolution];
		backNormalMap = new float[resolution * 3];
		std::cout << "framebuffer window size:(" << scr_width << ", " << scr_height << ")\n";

		// framebuffer configuration
		// -------------------------
		uint framebuffer;
		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		// create a color attachment texture
		uint textureColorbuffer;
		glGenTextures(1, &textureColorbuffer);
		glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, scr_width, scr_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
		// create a depth attachment texture
		uint textureDepthbuffer;
		glGenTextures(1, &textureDepthbuffer);
		glBindTexture(GL_TEXTURE_2D, textureDepthbuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, scr_width, scr_height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textureDepthbuffer, 0);
		// now that we actually created the framebuffer and added all attachments, we want to check if it is actually complete now 
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
		}

		// generate depth and normal maps for the front camera
		createMap(textureColorbuffer, textureDepthbuffer, window, frontCamera, frontDepthMap, frontNormalMap);

		// generate depth and normal maps for the back camera
		createMap(textureColorbuffer, textureDepthbuffer, window, backCamera, backDepthMap, backNormalMap);

		// normal coords transformation
		for (size_t i = 0; i < resolution * 3; i += 1) {
			frontNormalMap[i] = 2 * frontNormalMap[i] - 1;
			backNormalMap[i] = 2 * backNormalMap[i] - 1;
		}

		glDeleteFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glUseProgram(0);
	}

	void flush(Camera* camera)
	{
		runtimeShader.use();
		runtimeShader.setMat4("projection", camera->GetPerspectiveProjectionMatrix());
		runtimeShader.setMat4("view", camera->GetViewMatrix());
		model->Draw(runtimeShader);
		glUseProgram(0);
	}

	/*
	 * point collision detection with model
	 * if collided, return model's normol vector at colliding point
	 */
	bool collideWithModel(const glm::vec3& point)
	{
		if (!model->collisionBox.collideWithPoint(point)) {
			return false;
		}

		glm::vec3 frontPos = model->collisionBox.getFrontPosition(point);
		glm::vec3 backPos = model->collisionBox.getBackPosition(point);

		float z_front = getDepth(frontPos, frontDepthMap);
		float z_back = getDepth(backPos, backDepthMap);

		return (frontPos.z >= z_front && backPos.z >= z_back);
	}

	void collisionResponse(Node *node, const glm::vec3& clothPos)
	{
		// std::cout << "collided\n";
		glm::vec3 currPosition = node->position + clothPos;
		glm::vec3 lastPosition = node->lastPosition + clothPos;
		
		glm::vec3 vn;
		glm::vec3 vt;
		glm::vec3 normal;
		glm::vec3 velocity = node->velocity;
		if (model->collisionBox.onFrontSide(currPosition))
		{
			// 与模型前部碰撞
			glm::vec3 frontPos = model->collisionBox.getFrontPosition(currPosition);
			normal = getNormal(frontPos, frontNormalMap);
		}
		else
		{
			// 与模型后部碰撞
			glm::vec3 backPos = model->collisionBox.getBackPosition(currPosition);
			normal = getNormal(backPos, backNormalMap);
		}
		float cos = glm::dot(normal, node->velocity) / (glm::length(normal) * glm::length(node->velocity));
		float sin = glm::sqrt(1 - cos * cos);
		vn = velocity * cos;
		vt = velocity * sin;
		float friction = 0.8;
		float reflection = 0.2;
		// node->velocity = friction * vt - reflection * vn;
		node->position = node->position + normal * 0.03f;
		node->velocity *= -1.0f;
		// std::cout << "update velocity: " << node->velocity.x << " " << node->velocity.y << " " << node->velocity.z << "\n";

		// glm::vec3 currPosition = node->position + clothPos;
		// glm::vec3 lastPosition = node->lastPosition + clothPos;
		// // std::cout <<"current: " << currPosition.x << " " << currPosition.y << " " << currPosition.z << std::endl;
		// // std::cout << "last: " << lastPosition.x << " " << lastPosition.y << " " << lastPosition.z << std::endl;
		// 
		// // 转换为前部图像坐标
		// glm::vec3 currFrontPos = model->collisionBox.getFrontPosition(currPosition);
		// glm::vec3 lastFrontPos = model->collisionBox.getFrontPosition(lastPosition);
		// 
		// // 在前部深度图像上确定一条直线(在后部深度图像上也能生成, 但计算出来的点是相同的)
		// float x1 = currFrontPos.x;
		// float y1 = currFrontPos.y;
		// float z1 = currFrontPos.z;
		// float x2 = lastFrontPos.x;
		// float y2 = lastFrontPos.y;
		// float z2 = lastFrontPos.z;
		// // todo: divide by zero
		// float k1 = (y2 - y1) / (x2 - x1);
		// float b1 = y1 - (y2 - y1) * x1 / (x2 - x1);
		// float k2 = (z2 - z1) / (x2 - x1);
		// float b2 = z2 - (z2 - z1) * x1 / (x2 - x1);
		// 
		// vector<glm::vec3> candidate;
		// 
		// // 计算直线上的点
		// for (float x = min(x1, x2); x <= max(x1, x2); x++)
		// {
		// 	float y = k1 * x + b1;
		// 	float z = k2 * x + b2;
		// 	if (y > 512) continue;
		// 	// 该点为碰撞点
		// 	if (z - getDepth(glm::vec2(x, y), frontDepthMap) < 1e-3) {
		// 		candidate.push_back(glm::vec3(x, y, z));
		// 	}
		// }
		// 
		// float t;
		// float disToLastPosition = FLT_MAX;
		// glm::vec3 normal = glm::vec3(0.0f);
		// glm::vec3 collisionPoint = lastFrontPos;
		// for (const glm::vec3& point : candidate)
		// {
		// 	// t = distance(point, currFrontPos) / distance(currFrontPos, lastFrontPos);
		// 	if (glm::distance(point, lastFrontPos) < disToLastPosition)
		// 	{
		// 		collisionPoint = point;
		// 		disToLastPosition = glm::distance(collisionPoint, lastFrontPos);
		// 		normal = getNormal(collisionPoint, frontNormalMap);
		// 	}
		// }
		// 
		// node->position = model->collisionBox.inverseFrontPosition(collisionPoint) - clothPos;
		// glm::vec3 vn = normal * glm::dot(node->velocity, normal);
		// node->velocity *= -1.3f;
		// if (normal == glm::vec3(0.0f)) node->velocity *= -1.0f;
		// else node->velocity = (node->velocity - vn) * (1.0f - 0.1f) - vn * 0.8f;
	}

private:
	int scr_width;
	int scr_height;
	Model* model;
	Shader runtimeShader;
	Shader offlineShader;
	float* frontDepthMap;
	float* backDepthMap;
	float* frontNormalMap;
	float* backNormalMap;

	void init()
	{
		runtimeShader = Shader("resources/Shaders/ModelVS.glsl", "resources/Shaders/ModelFS.glsl");
		offlineShader = Shader("resources/Shaders/offscreenVS.glsl", "resources/Shaders/offscreenFS.glsl");

		// model matrix
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -3.0f, -2.5f));      // translate it down so it's at the center of the scene
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.08f, 0.08f, 0.08f));	       // it's a bit too big for our scene, so scale it down

		model->collisionBox.toWorldPosition(modelMatrix);  // change the position of AABB box accordingly

		runtimeShader.use();
		runtimeShader.setMat4("model", modelMatrix);

		offlineShader.use();
		offlineShader.setMat4("model", modelMatrix);

		glUseProgram(0);
	}

	/*
	 * create depth map and normal map
	 */
	void createMap(uint colorbuffer, uint depthbuffer, GLFWwindow* window, Camera* camera, float* depthMap, float* normalMap)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		offlineShader.use();
		offlineShader.setMat4("view", camera->GetViewMatrix());
		offlineShader.setMat4("projection", camera->GetOrthoProjectionMatrix());
		model->Draw(offlineShader);

		// read back depth values and normals
		// todo: what is the order of coords?
		glfwSwapBuffers(window);
		glBindTexture(GL_TEXTURE_2D, depthbuffer);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, depthMap);
		glBindTexture(GL_TEXTURE_2D, colorbuffer);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, normalMap);
	}

	/*
	 * point 是图像坐标系统下的坐标, 是由 collisionBox.getFrontPosition/getBackPosition 返回的
	 */
	float getDepth(const glm::vec2& point, float* depthMap) const
	{
		int x = point.x;
		int y = point.y;
		return depthMap[y * scr_width + x];
	}

	/*
	 * point 是图像坐标系统下的坐标, 是由 collisionBox.getFrontPosition/getBackPosition 返回的
	 */
	glm::vec3 getNormal(const glm::vec2& point, float* normalMap) const
	{
		int x = point.x;
		int y = point.y;
		int index = 3 * (y * scr_width + x);
		return glm::vec3(normalMap[index], normalMap[index + 1], normalMap[index + 2]);
	}

	float distance(const glm::vec2& p1, const glm::vec2& p2)
	{
		return glm::distance(p1, p2);
	}
};