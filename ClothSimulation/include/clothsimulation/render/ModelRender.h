#pragma once

#include "../Model.h"
#include "../Rigid.h"

class ModelRender
{
public:
	Model* model;
	Shader shader;
	vector<Ball*> collisionBall;

	ModelRender(Model* model)
	{
		this->model = model;
		this->init();
	}

	void flush()
	{
		shader.use();
		shader.setMat4("projection", camera.GetProjectionMatrix());
		shader.setMat4("view", camera.GetViewMatrix());
		model->Draw(shader);
		glUseProgram(0);
	}

private:
	void init()
	{
		shader = Shader("resources/Shaders/ModelVS.glsl", "resources/Shaders/ModelFS.glsl");
		shader.use();

		// model matrix
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -3.0f, -2.5f));      // translate it down so it's at the center of the scene
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.08f, 0.08f, 0.08f));	       // it's a bit too big for our scene, so scale it down

		model->collisionBox.toWorldPosition(modelMatrix);
		shader.setMat4("model", modelMatrix);
		shader.setMat4("projection", camera.GetProjectionMatrix());
		shader.setMat4("view", camera.GetViewMatrix());

		glUseProgram(0);
	}
};