#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "clothsimulation/Cloth.h"
#include "clothsimulation/Rigid.h"
#include "clothsimulation/Display.h"
#include "clothsimulation/Model.h"

#define SCR_WIDTH 800
#define SCR_HEIGHT 800

#define TIME_STEP 0.01

/** Executing Flow **/
int running = 1;

/** Functions **/
void processInput(GLFWwindow* window);

/** Callback functions **/
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_position_callback(GLFWwindow* window, double xpos, double ypos);

/** Global **/
// Wind
int windForceScale = 15;

// Cloths
int clothNumber = 0;
Vec2 clothSize(6, 6);
//			 Position           Size       clothID
Cloth cloth1(Vec3(-3, 7.5, -2), clothSize, ++clothNumber);
Cloth cloth2(Vec3(-3, 7.5, -4), clothSize, ++clothNumber);
std::vector<Cloth*> cloths = { &cloth1, &cloth2 };
Cloth* selectedCloth = nullptr; // 需要移动的衣片

// Ground
Vec3 groundPos(-5, 1.5, 0);
Vec2 groundSize(10, 10);
glm::vec4 groundColor(0.8, 0.8, 0.8, 1.0);
Ground ground(groundPos, groundSize, groundColor);

// Ball
// Vec3 ballPos(0, 3, -2);
// int ballRadius = 2;
// glm::vec4 ballColor(0.6f, 0.5f, 0.8f, 1.0f);
// Ball ball(ballPos, ballRadius, ballColor);

// Window and world
GLFWwindow* window;
Vec3 bgColor = Vec3(50.0 / 255, 50.0 / 255, 60.0 / 255);
Vec3 gravity(0.0, -9.8 / Cloth::iterationFreq, 0.0);

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// initial mouse position
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

int main(int argc, const char* argv[])
{
	/** Prepare for rendering **/
	// Initialize GLFW
	glfwInit();
	// Set OpenGL version number as 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Use the core profile
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// MacOS is forward compatible
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	/** Create a GLFW window **/
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Cloth Simulation", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window." << std::endl;
		glfwTerminate();
		return -1;
	}
	// Set the context of this window as the main context of current thread
	glfwMakeContextCurrent(window);

	// Initialize GLAD : this should be done before using any openGL function
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD." << std::endl;
		glfwTerminate(); // This line isn't in the official source code, but I think that it should be added here.
		return -1;
	}

	/** Register callback functions **/
	// Callback functions should be registered after creating window and before initializing render loop
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, mouse_position_callback);

	/** Generate Object Renderers, Add Initial Force to Cloths **/
	vector<ClothRender> clothRenders;
	vector<ClothSpringRender> clothSpringRenders;
	Vec3 initForce(10.0, 40.0, 20.0);
	for (Cloth* cloth : cloths)
	{
		cloth->addForce(initForce);
		clothRenders.push_back(ClothRender(cloth));
		clothSpringRenders.push_back(ClothSpringRender(cloth));
	}

	GroundRender groundRender(&ground);
	// BallRender ballRender(&ball);

	glEnable(GL_DEPTH_TEST);
	glPointSize(1);

	// load model shader and .obj
	// Shader ourShader("resources/Shaders/ModelVS.glsl", "resources/Shaders/ModelFS.glsl");
	// Model ourModel("resources/Models/nanosuit/nanosuit.obj");

	/** Redering loop **/
	running = 1;
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		/** Check for events **/
		processInput(window);

		/** Set background color **/
		glClearColor(bgColor.x, bgColor.y, bgColor.z, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/** -------------------------------- Simulation & Rendering -------------------------------- **/
		for (size_t i = 0; i < cloths.size(); i += 1)
		{
			Cloth* cloth = cloths[i];
			if (running) {
				for (int i = 0; i < Cloth::iterationFreq; i++)
				{
					cloth->computeForce(TIME_STEP, gravity);
					cloth->integrate(TIME_STEP);
					cloth->collisionResponse(&ground);
				}
				cloth->computeNormal();
			}

			/** Display **/
			if (Cloth::drawMode == DRAW_LINES)
			{
				clothSpringRenders[i].flush();
			}
			else
			{
				clothRenders[i].flush();
			}
		}
		// ballRender.flush();
		groundRender.flush();
		/** -------------------------------- Simulation & Rendering -------------------------------- **/

		/** --------------------------------    3D Model Loading    -------------------------------- **/
		// draw model
		// ourShader.use();
		// // view/projection transformations
		// glm::mat4 projection = camera.GetProjectionMatrix();
		// glm::mat4 view = camera.GetViewMatrix();
		// ourShader.setMat4("projection", projection);
		// ourShader.setMat4("view", view);
		// // render the loaded model
		// glm::mat4 model = glm::mat4(1.0f);
		// model = glm::translate(model, glm::vec3(0.0f, 2.0f, -2.0f)); // translate it down so it's at the center of the scene
		// model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));	      // it's a bit too big for our scene, so scale it down
		// ourShader.setMat4("model", model);
		// ourModel.Draw(ourShader);
		/** --------------------------------    3D Model Loading    -------------------------------- **/

		glfwSwapBuffers(window);
		glfwPollEvents(); // Update the status of window
	}

	glfwTerminate();

	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	camera.SetAspect(width, height);
	glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// 3D Picking: https://antongerdelan.net/opengl/raycasting.html
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		// 衣服上一点经过 MVP 变换后的坐标，为什么在裁剪空间中的坐标范围不是 [-1,1]?
		// glm::vec3 clothPos = glm::vec3(-3, 7.5, -2);
		// glm::mat4 model = glm::mat4(1.0f);
		// model = glm::translate(model, glm::vec3(clothPos.x, clothPos.y, clothPos.z));
		// glm::vec4 pos = camera.GetProjectionMatrix() * camera.GetViewMatrix() * model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		// std::cout << "cloth_x: " << pos.x / pos.w << " cloth_y: " << pos.y / pos.w << " cloth_z: " << pos.z / pos.w << std::endl;
		// 2D Viewport Coordinates
		// get the mouse x,y pixel coordinates
		// range [0:width, height:0]
		double mouse_x, mouse_y;

		glfwGetCursorPos(window, &mouse_x, &mouse_y);
		std::cout << "mouse_x: " << mouse_x << " mouse_y: " << mouse_y << std::endl;

		// 3D Normalised Device Coordinates
		// scale the range of x,y and reverse the direction of y
		// we don't actually need to specify a z yet, but I put one in (for the craic)
		// range [-1:1, -1:1, -1:1]
		// TODO: SCR_WIDTH 要随窗口大小改变而改变
		float x = (2.0f * mouse_x) / (double)SCR_WIDTH - 1.0f;
		float y = 1.0f - (2.0f * mouse_y) / (double)SCR_HEIGHT;
		float z = 1.0f;
		glm::vec3 ray_nds = glm::vec3(x, y, z);
		std::cout << "nds_x: " << x << " nds_y: " << y << " nds_z: " << z << std::endl;

		// 4D Homogeneous Clip Coordinates
		// We want our ray's z to point forwards - this is usually the negative z direction in OpenGL style
		// range [-1:1, -1:1, -1:1, -1:1]
		glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, -1.0, 1.0);
		std::cout << "clip_x: " << ray_clip.x << " clip_y: " << ray_clip.y << " clip_z: " << ray_clip.z << std::endl;

		// 4D Eye(Camera) Coordinates
		// Normally, to get into clip space from eye space we multiply the vector by a projection matrix
		// We can go backwards by multiplying by the inverse of this matrix
		glm::vec4 ray_eye = glm::inverse(camera.GetProjectionMatrix()) * ray_clip;
		// Now, we only needed to un-project the x,y part, so let's manually set the z,w part to mean "forwards, and not a point"
		ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);
		std::cout << "eye_x: " << ray_eye.x << " eye_y: " << ray_eye.y << " eye_z: " << ray_eye.z << std::endl;

		// 4D World Coordinates
		glm::vec4 ray_temp = glm::inverse(camera.GetViewMatrix()) * ray_eye;
		glm::vec3 ray_world = glm::vec3(ray_temp.x, ray_temp.y, ray_temp.z);
		ray_world = glm::normalize(ray_world);
		std::cout << "world_x: " << ray_world.x * ray_temp.w << " world_y: " << ray_world.y * ray_temp.w << " world_z: " << ray_world.z * ray_temp.w << std::endl;

		// vector from camera to ray_world
		glm::vec3 ray = ray_world - camera.Position;
		glm::vec3 pointLeftUpper = glm::vec3(cloths[0]->clothPos.x, cloths[0]->clothPos.y, cloths[0]->clothPos.z);
		glm::vec3 pointRightUpper = pointLeftUpper + glm::vec3(cloths[0]->width, 0, 0);
		glm::vec3 pointRightBottom = pointLeftUpper + glm::vec3(cloths[0]->width, -cloths[0]->height, 0);
		std::cout << "x1: " << pointLeftUpper.x << " y1: " << pointLeftUpper.y << " z1: " << pointLeftUpper.z << std::endl;
		std::cout << "x2: " << pointRightUpper.x << " y2: " << pointRightUpper.y << " z2: " << pointRightUpper.z << std::endl;
		std::cout << "x3: " << pointRightBottom.x << " y3: " << pointRightBottom.y << " z3: " << pointRightBottom.z << std::endl;
		// 平面的法向量, 和平面的交点是 pointLeftUpper
		glm::vec3 normal = glm::cross(pointRightBottom - pointLeftUpper, pointRightUpper - pointLeftUpper);
		// std::cout << "normal x: " << normal.x << " y: " << normal.y << " z: " << normal.z << std::endl;
		double t = glm::dot(pointLeftUpper - camera.Position, normal) / glm::dot(ray, normal);
		glm::vec3 intersect = camera.Position + glm::vec3(ray.x * t, ray.y * t, ray.z * t);
		std::cout << "t: " << t << std::endl;
		std::cout << "intersect x: " << intersect.x << " y: " << intersect.y << " z: " << intersect.z << std::endl;
		if (intersect.x >= pointLeftUpper.x && intersect.x <= pointRightUpper.x && intersect.y <= pointLeftUpper.y && intersect.y >= pointRightBottom.y)
			std::cout << "hit!\n\n";
		else
			std::cout << "miss!\n\n";

	}
	// Start Sewing
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && running)
	{
		std::cout << "Start Sewing\n";
		Cloth* cloth1 = cloths[0];
		Cloth* cloth2 = cloths[1];
		double deltaZ = std::abs(cloth1->clothPos.z - cloth2->clothPos.z) / 2;
		std::cout << "deltaZ " << deltaZ << std::endl;
		for (int i = 0; i < cloth1->nodesPerRow; i++) {
			Node* n1 = cloth1->getNode(i, 0);
			Node* n2 = cloth2->getNode(i, 0);
			// std::cout << n1->position.x << " " << n1->position.y << " " << n1->position.z << std::endl;
			// std::cout << n2->position.x << " " << n2->position.y << " " << n2->position.z << std::endl;
			n1->position.z -= deltaZ;
			n2->position.z += deltaZ;
		}
	}
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	// if (firstMouse)
	// {
	// 	lastX = xpos;
	// 	lastY = ypos;
	// 	firstMouse = false;
	// }
	// 
	// float xoffset = xpos - lastX;
	// float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
	// 
	// lastX = xpos;
	// lastY = ypos;
	// 
	// camera.ProcessMouseMovement(xoffset, yoffset);
}

void processInput(GLFWwindow* window)
{
	/** Keyboard control **/ // If key did not get pressed it will return GLFW_RELEASE
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	/** Set draw mode **/
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
		Cloth::modifyDrawMode(DRAW_NODES);
	}
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
		Cloth::modifyDrawMode(DRAW_LINES);
	}
	if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
		Cloth::modifyDrawMode(DRAW_FACES);
	}

	/** control : [W] [S] [A] [D] **/
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		camera.ProcessKeyboard(FORWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		camera.ProcessKeyboard(LEFT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		camera.ProcessKeyboard(RIGHT, deltaTime);
	}

	/** Pause simulation **/
	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
		running = 0;
		printf("Paused.\n");
	}
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
		running = 1;
		printf("Running..\n");
	}

	/** Select Cloths **/
	// TODO: 不能用数字选衣服的方法。如果衣服数量很多怎么办？
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
		for (Cloth* cloth : cloths)
		{
			if (cloth->clothID == 1)
			{
				selectedCloth = cloth;
				std::cout << "select cloth 1\n";
				break;
			}
		}
	}
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
		for (Cloth* cloth : cloths)
		{
			if (cloth->clothID == 2)
			{
				selectedCloth = cloth;
				std::cout << "select cloth 2\n";
				break;
			}
		}
	}

	/** Move Cloth **/
	if (selectedCloth != nullptr)
	{
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && running) {
			selectedCloth->move(Vec3(0.0f, 0.2f, 0.0f));
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && running) {
			selectedCloth->move(Vec3(0.0f, -0.2f, 0.0f));
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && running) {
			selectedCloth->move(Vec3(-0.2f, 0.0f, 0.0f));
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && running) {
			selectedCloth->move(Vec3(0.2f, 0.0f, 0.0f));
		}
	}
}
