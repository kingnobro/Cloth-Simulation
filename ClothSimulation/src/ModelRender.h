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

    /*
     * ������Ⱦ, �������ͼ�ͷ���ͼ, ������ײ������Ӧ
     */
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

    /*
     * ÿ����Ⱦѭ����������
     */
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
     */
    bool collideWithModel(const glm::vec3& point)
    {
        // ���ж��Ƿ�����ײ����
        if (!model->collisionBox.collideWithPoint(point)) {
            return false;
        }

        // ��ȡ���������(ǰ����)����ϵͳ�µ�����
        glm::vec3 frontPos = model->collisionBox.getFrontPosition(point);
        glm::vec3 backPos = model->collisionBox.getBackPosition(point);

        float z_front = getDepth(frontPos, frontDepthMap);
        float z_back = getDepth(backPos, backDepthMap);

        // std::cout << "frontPos.z:" << frontPos.z << " z_front:" << z_front << " backPos.z:" << backPos.z << " z_back:" << z_back << "\n";

        float tolerance = 0.05f;
        return (frontPos.z >= z_front - tolerance && backPos.z >= z_back - tolerance);
    }

    /*
     * ��ײ��Ӧ
     * node: ��⵽��ײ���ʵ�
     * modelVector: �� node �ľֲ�����ת��Ϊ�������������
     */
    void collisionResponse(Node* node, const glm::vec3& modelVector)
    {
        glm::vec3 currPosition = node->position + modelVector;
        glm::vec3 currFrontPosition = model->collisionBox.getFrontPosition(currPosition);
        glm::vec3 currBackPosition = model->collisionBox.getBackPosition(currPosition);

        // ��ȡ��ײ�㴦�ķ���
        // ���� [x, y] ��Ӧģ�� ǰ�� �� �� ��������, ������Ҫ�ж� [x, y] ��ǰ�����Ǻ󲿸���
        float z_front = getDepth(currFrontPosition, frontDepthMap);
        float z_back = getDepth(currBackPosition, backDepthMap);
        glm::vec3 normal = fabs(currFrontPosition.z - z_front) < fabs(currBackPosition.z - z_back) ?
            getNormal(currFrontPosition, frontNormalMap) :
            getNormal(currBackPosition, backNormalMap);

        // ���ʵ����ŵ�ǰ��������ƽ��һ�ξ���
        float epsilon = 0.03f;
        node->position = node->position + normal * epsilon;
        // ���ٶ�ȡ��
        node->velocity *= -0.001f;
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
        runtimeShader = Shader("src/shaders/ModelVS.glsl", "src/shaders/ModelFS.glsl");
        offlineShader = Shader("src/shaders/offscreenVS.glsl", "src/shaders/offscreenFS.glsl");

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
        glfwSwapBuffers(window);
        glBindTexture(GL_TEXTURE_2D, depthbuffer);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, depthMap);
        glBindTexture(GL_TEXTURE_2D, colorbuffer);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, normalMap);
    }

    /*
     * ��ȡ���
     * point ��ͼ������ϵͳ�µ�����, ���� collisionBox.getFrontPosition/getBackPosition ���ص�
     */
    float getDepth(const glm::vec2& point, float* depthMap) const
    {
        int x = point.x;
        int y = point.y;
        return depthMap[y * scr_width + x];
    }

    /*
     * ��ȡ����
     * point ��ͼ������ϵͳ�µ�����, ���� collisionBox.getFrontPosition/getBackPosition ���ص�
     */
    glm::vec3 getNormal(const glm::vec2& point, float* normalMap) const
    {
        int x = point.x;
        int y = point.y;
        int index = 3 * (y * scr_width + x);
        return glm::vec3(normalMap[index], normalMap[index + 1], normalMap[index + 2]);
    }
};