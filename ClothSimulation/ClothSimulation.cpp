﻿#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "Headers/stb_image.h"
#include "Headers/Cloth.h"
#include "Headers/Rigid.h"
#include "Headers/Display.h"
#include "Headers/Model.h"

#define WIDTH 800
#define HEIGHT 800

#define AIR_FRICTION 0.02
#define TIME_STEP 0.01

/** Executing Flow **/
int running = 1;

/** Functions **/
void processInput(GLFWwindow* window);

/** Callback functions **/
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);

/** Global **/
// Wind
int windBlowing = 0;
int windForceScale = 15;
Vec3 windStartPos;
Vec3 windDir;
Vec3 wind;
// Cloth
Vec3 clothPos(-3, 7.5, -2);
Vec2 clothSize(6, 6);
Cloth cloth(clothPos, clothSize);
// Ground
Vec3 groundPos(-5, 1.5, 0);
Vec2 groundSize(10, 10);
glm::vec4 groundColor(0.8, 0.8, 0.8, 1.0);
Ground ground(groundPos, groundSize, groundColor);
// Ball
Vec3 ballPos(0, 3, -2);
int ballRadius = 1;
glm::vec4 ballColor(0.6f, 0.5f, 0.8f, 1.0f);
Ball ball(ballPos, ballRadius, ballColor);
// Window and world
GLFWwindow* window;
Vec3 bgColor = Vec3(50.0 / 255, 50.0 / 255, 200.0 / 255);
Vec3 gravity(0.0, -9.8 / cloth.iterationFreq, 0.0);

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
    window = glfwCreateWindow(WIDTH, HEIGHT, "Cloth Simulation", NULL, NULL);
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
    glfwSetCursorPosCallback(window, cursor_pos_callback);

    /** Renderers **/
    ClothRender clothRender(&cloth);
    ClothSpringRender clothSpringRender(&cloth);
    GroundRender groundRender(&ground);
    BallRender ballRender(&ball);

    Vec3 initForce(10.0, 40.0, 20.0);
    cloth.addForce(initForce);

    glEnable(GL_DEPTH_TEST);
    glPointSize(3);

    Shader ourShader("Shaders/ModelVS.glsl", "Shaders/ModelFS.glsl");
    Model ourModel("Models/nanosuit/nanosuit.obj");

    /** Redering loop **/
    running = 1;
    while (!glfwWindowShouldClose(window))
    {
        /** Check for events **/
        processInput(window);

        /** Set background color **/
        glClearColor(bgColor.x, bgColor.y, bgColor.z, 1.0); // Set color value (R,G,B,A) - Set Status
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /** -------------------------------- Simulation & Rendering -------------------------------- **/

        // if (running) {
        //     for (int i = 0; i < cloth.iterationFreq; i++) {
        //         cloth.computeForce(TIME_STEP, gravity);
        //         cloth.integrate(AIR_FRICTION, TIME_STEP);
        //         cloth.collisionResponse(&ground, &ball);
        //     }
        //     cloth.computeNormal();
        // }
        // 
        // /** Display **/
        // if (cloth.drawMode == Cloth::DRAW_LINES) {
        //     clothSpringRender.flush();
        // }
        // else {
        //     clothRender.flush();
        // }
        // ballRender.flush();
        // groundRender.flush();

        // draw model
        ourShader.use();
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = glm::mat4(1.0f);
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -40.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        ourModel.Draw(ourShader);

        /** -------------------------------- Simulation & Rendering -------------------------------- **/

        glfwSwapBuffers(window);
        glfwPollEvents(); // Update the status of window
    }

    glfwTerminate();

    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && running) // Start wind
    {
        windBlowing = 1;
        // Set start point of wind direction
        windStartPos.setZeroVec();
        glfwGetCursorPos(window, &windStartPos.x, &windStartPos.y);
        windStartPos.y = -windStartPos.y; // Reverse y since the screen local in the fourth quadrant
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE && running) // End wind
    {
        windBlowing = 0;
        windDir.setZeroVec();
    }
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    /** Wind **/
    if (windBlowing && running) {
        windDir = Vec3(xpos, -ypos, 0) - windStartPos;
        windDir.normalize();
        wind = windDir * windForceScale;
        cloth.addForce(wind);
    }
}

void processInput(GLFWwindow* window)
{
    /** Keyboard control **/ // If key did not get pressed it will return GLFW_RELEASE
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    /** Set draw mode **/
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        cloth.drawMode = Cloth::DRAW_NODES;
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        cloth.drawMode = Cloth::DRAW_LINES;
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        cloth.drawMode = Cloth::DRAW_FACES;
    }

    /** 
    control : [W] [S] [A] [D] [Q] [E] **/
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        cam.pos.y += cam.speed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        cam.pos.y -= cam.speed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        cam.pos.x -= cam.speed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        cam.pos.x += cam.speed;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        cam.pos.z -= cam.speed;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        cam.pos.z += cam.speed;
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

    /** Drop the cloth **/
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS && running) {
        cloth.unPin(cloth.pin1);
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && running) {
        cloth.unPin(cloth.pin2);
    }

    /** Pull cloth **/
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && running) {
        cloth.addForce(Vec3(0.0, 0.0, -windForceScale));
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && running) {
        cloth.addForce(Vec3(0.0, 0.0, windForceScale));
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && running) {
        cloth.addForce(Vec3(-windForceScale, 0.0, 0.0));
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && running) {
        cloth.addForce(Vec3(windForceScale, 0.0, 0.0));
    }
}
