#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Display.h"

#include "ClothRender.h"
#include "MeshRender.h"
#include "ModelRender.h"

#define TIME_STEP 0.01

int scr_width = 600;
int scr_height = 600;
const int iterationFreq = 7;


/** Functions **/
void processInput(GLFWwindow* window);

/** Callback functions **/
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_position_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// Window and world
GLFWwindow* window;
glm::vec3 backgroundColor = glm::vec3(105.0 / 255, 105.0 / 255, 105.0 / 255);

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// initial mouse position
float lastX = scr_width / 2.0f;
float lastY = scr_height / 2.0f;
bool firstMouse = true;
bool right_bottom_down = false;

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
    glfwSetScrollCallback(window, scroll_callback);

    /** Generate Object Renderers **/
    std::vector<ClothRender> clothRenders;
    std::vector<ClothSpringRender> clothSpringRenders;
    for (Cloth* cloth : cloths)
    {
        clothRenders.push_back(ClothRender(cloth));
        clothSpringRenders.push_back(ClothSpringRender(cloth));
    }
    // Model
    Model ourModel("assets/models/man/man_body.obj");
    ModelRender modelRender(&ourModel);

    glEnable(GL_DEPTH_TEST);
    glPointSize(3);

    // offscreen render, to generate depth maps and normal maps for collision detection and response
    modelRender.offScreenRender(
        &(ourModel.collisionBox.frontCamera), 
        &(ourModel.collisionBox.backCamera),
        window
    );

    // cloth self collision
    std::cout << "sphereR: " << clthCollid.sphereR << std::endl;
    std::cout << "cellUnit: " << clthCollid.cellUnit << std::endl;
    std::cout << "spaceDim: " << clthCollid.spaceDim.x << " " << clthCollid.spaceDim.y << " " << clthCollid.spaceDim.z << std::endl;

    /** Redering loop **/
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        /** Check for events **/
        processInput(window);

        /** Set background color **/
        glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /** -------------------------------- Simulation & Rendering -------------------------------- **/
        for (size_t i = 0; i < cloths.size(); i += 1)
        {
            Cloth* cloth = cloths[i];
            for (int iter = 0; iter < iterationFreq; iter++) {
                if (cloth->collisionCount < MAX_COLLISION_TIME) {
                    cloth->update((float)TIME_STEP);
                    if (cloth->isSewed) {
                        cloth->modelCollision(modelRender);
                        //cloth->clothCollision(&clthCollid);
                    }
                }
            }
        
            /** Display **/
            if (Cloth::drawMode == DRAW_LINES) {
                clothSpringRenders[i].update(&camera);
            }
            else {
                clothRenders[i].update(&camera);
            }
        }
        sewMachine.update((float)TIME_STEP);
        modelRender.flush(&camera);
        sewMachine.drawSewingLine(camera.GetViewMatrix(), camera.GetPerspectiveProjectionMatrix()); // sewing line
        /** -------------------------------- Simulation & Rendering -------------------------------- **/

        glfwSwapBuffers(window);
        glfwPollEvents(); // Update the status of window
    }

    glfwTerminate();

    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    scr_width = width;
    scr_height = height;
    camera.SetAspect(scr_width, scr_height);
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    double mouse_x, mouse_y;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);

    // 3D Picking
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        glm::vec3 ray = mouseRay.calculateMouseRay(mouse_x, mouse_y, (int)scr_width, (int)scr_height);
        selectedCloth = clothPicker.pickCloth(cloths, ray);
        sewMachine.setCandidateCloth(selectedCloth);
    }
    // rotation
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        right_bottom_down = true;
        lastX = mouse_x;
        lastY = mouse_y;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        right_bottom_down = false;
    }
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (!right_bottom_down) {
        return;
    }

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

/*
 * scroll can only generate yoffset, xoffset thus is always 0  
 */
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
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
        std::cout << "Mode: Draw Nodes\n";
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        Cloth::modifyDrawMode(DRAW_LINES);
        std::cout << "Mode: Draw Lines\n";
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        Cloth::modifyDrawMode(DRAW_FACES);
        std::cout << "Mode: Draw Faces\n";
    }

    /** control : [W] [S] [A] [D] **/
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(UP, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(DOWN, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }

    /** Move Cloth **/
    if (selectedCloth != nullptr)
    {
        float moveStep = 0.05f;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            selectedCloth->moveCloth(glm::vec3(0.0f, moveStep, 0.0f));
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            selectedCloth->moveCloth(glm::vec3(0.0f, -moveStep, 0.0f));
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            selectedCloth->moveCloth(glm::vec3(-moveStep, 0.0f, 0.0f));
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            selectedCloth->moveCloth(glm::vec3(moveStep, 0.0f, 0.0f));
        }
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
            selectedCloth->moveCloth(glm::vec3(0.0f, 0.0f, moveStep));
        }
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
            selectedCloth->moveCloth(glm::vec3(0.0f, 0.0f, -moveStep));
        }
    }

    // reset sewing status
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        sewMachine.reset();
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        sewMachine.SewCloths();
    }
}
