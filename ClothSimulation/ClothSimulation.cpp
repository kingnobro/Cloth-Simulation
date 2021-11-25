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
#include "clothsimulation/MouseRay.h"
#include "clothsimulation/ClothPicker.h"

int scr_width = 800;
int scr_height = 800;

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
glm::vec2 clothSize(6, 6);
//			 Position           Size       clothID
Cloth cloth1(glm::vec3(-3, 7.5, -2), clothSize, ++clothNumber);
Cloth cloth2(glm::vec3(-3, 7.5, -4), clothSize, ++clothNumber);
std::vector<Cloth*> cloths = { &cloth1, &cloth2 };
Cloth* selectedCloth = nullptr; // 需要移动的衣片

// Ground
glm::vec3 groundPos(-5, 1.5, 0);
glm::vec2 groundSize(10, 10);
glm::vec4 groundColor(0.8, 0.8, 0.8, 1.0);
Ground ground(groundPos, groundSize, groundColor);

// Ball
// Vec3 ballPos(0, 3, -2);
// int ballRadius = 2;
// glm::vec4 ballColor(0.6f, 0.5f, 0.8f, 1.0f);
// Ball ball(ballPos, ballRadius, ballColor);

// Window and world
GLFWwindow* window;
glm::vec3 bgColor = glm::vec3(50.0 / 255, 50.0 / 255, 60.0 / 255);
glm::vec3 gravity(0.0, -9.8 / Cloth::iterationFreq, 0.0);

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// initial mouse position
float lastX = scr_width / 2.0f;
float lastY = scr_height / 2.0f;
bool firstMouse = true;

// 3D raycast picker
MouseRay mouseRay = MouseRay(&camera);
ClothPicker clothPicker = ClothPicker(&camera);

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
	window = glfwCreateWindow(scr_width, scr_height, "Cloth Simulation", NULL, NULL);
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
	glm::vec3 initForce(10.0, 40.0, 20.0);
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
	scr_width = width;
	scr_height = height;
	glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// 3D Picking
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		double mouse_x, mouse_y;
		glfwGetCursorPos(window, &mouse_x, &mouse_y);

		glm::vec3 ray = mouseRay.calculateMouseRay(mouse_x, mouse_y, (int)scr_width, (int)scr_height);
		selectedCloth = clothPicker.pickCloth(cloths, ray);
	}
	// Sewing
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
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}
	
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
	
	lastX = xpos;
	lastY = ypos;
	
	camera.ProcessMouseMovement(xoffset, yoffset);
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

	/** Move Cloth **/
	if (selectedCloth != nullptr)
	{
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			selectedCloth->move(glm::vec3(0.0f, 0.2f, 0.0f));
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			selectedCloth->move(glm::vec3(0.0f, -0.2f, 0.0f));
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			selectedCloth->move(glm::vec3(-0.2f, 0.0f, 0.0f));
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			selectedCloth->move(glm::vec3(0.2f, 0.0f, 0.0f));
		}
	}
}
