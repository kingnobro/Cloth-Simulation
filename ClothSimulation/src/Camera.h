#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN,
};

enum Camera_Type
{
    Perspective,
    Orthographic,
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 13.0f;
const float SENSITIVITY = 0.2f;
const float ZOOM = 45.0f;
const float ASPECT = 1.0f;

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    Camera_Type Type;
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    glm::vec3 Focus;
    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    float Aspect;
    // orthograpic projection
    float left;
    float right;
    float bottom;
    float top;
    float near;
    float far;
    

    // constructor with vectors
    Camera(Camera_Type type = Perspective, glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM), Aspect(ASPECT)
    {
        Type = type;
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        Focus = glm::vec3(0.0f, 0.0f, -2.5f);   // TODO: fix hard code
        updateCameraVectors();
    }
    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    glm::mat4 GetPerspectiveProjectionMatrix()
    {
        return glm::perspective(glm::radians(Zoom), Aspect, 0.1f, 100.0f);
    }

    glm::mat4 GetOrthoProjectionMatrix()
    {
        return glm::ortho(left, right, bottom, top, near, far);
    }

    void SetOrthoBoundary(float left, float right, float bottom, float top, float near, float far) {
        this->left = left;
        this->right = right;
        this->bottom = bottom;
        this->top = top;
        this->near = near;
        this->far = far;
    }

    void SetAspect(int SCR_Width, int SCR_Height) {
        Aspect = (float)SCR_Width / (float)SCR_Height;
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == UP) {
            Position += WorldUp * velocity;
            Focus += WorldUp * velocity;
        }
        if (direction == DOWN) {
            Position -= WorldUp * velocity;
            Focus -= WorldUp * velocity;
        }
        if (direction == LEFT) {
            Position -= Right * velocity;
            Focus -= Right * velocity;
        }
        if (direction == RIGHT) {
            Position += Right * velocity;
            Focus += Right * velocity;
        }
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        // Arcball rotation
        // https://gamedev.stackexchange.com/questions/20758/how-can-i-orbit-a-camera-about-its-target-point
        glm::vec3 camFocusVector = Position - Focus;

        // rotate around two axis
        glm::mat4 rotation = glm::mat4(1.0f);
        rotation = glm::rotate(rotation, glm::radians(yoffset), Right);
        rotation = glm::rotate(rotation, glm::radians(-xoffset), Up);

        // new 
        glm::vec3 newCamFocusVector = rotation * glm::vec4(camFocusVector, 0.0f);

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        //    \  |  /
        //     \ | /    we use angle between Up and FocusVector as judgement
        //      \|/
        //       v
        if (constrainPitch)
        {
            float cos = glm::dot(newCamFocusVector, WorldUp) / (glm::length(newCamFocusVector) * glm::length(WorldUp));
            if (1.0f - cos < 0.005f) {
                newCamFocusVector = camFocusVector;
            }
        }

        // new position after rotation
        Position = newCamFocusVector + Focus;

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

private:
    // calculates the front std::vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // Orthographic camera should not update Front, which is initialized in constructor
        if (Type == Perspective) {
            // calculate the new Front vector
            glm::vec3 front = Focus - Position;
            Front = glm::normalize(front);
        }
        else {
            glm::vec3 front;
            front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
            front.y = sin(glm::radians(Pitch));
            front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
            Front = glm::normalize(front);
        }
        // also re-calculate the Right and Up vector
        Right = glm::normalize(glm::cross(Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        Up = glm::normalize(glm::cross(Right, Front));
    }
};

#endif