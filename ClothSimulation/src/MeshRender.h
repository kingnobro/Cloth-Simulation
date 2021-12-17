#pragma once

#include "Spring.h"

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
    void init(Cloth* cloth, glm::vec4 c)
    {
        this->springs = cloth->springs;
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
            // std::cout << node1->worldPosition.x << " " << node1->worldPosition.y << " " << node1->worldPosition.z << "\n";
            // std::cout << node2->worldPosition.x << " " << node2->worldPosition.y << " " << node2->worldPosition.z << "\n\n";
            vboPos[i * 2] = node1->worldPosition;
            vboPos[i * 2 + 1] = node2->worldPosition;
            vboNor[i * 2] = node1->normal;
            vboNor[i * 2 + 1] = node2->normal;
        }

        /** Build shader **/
        shader = Shader("src/shaders/SpringVS.glsl", "src/shaders/SpringFS.glsl");
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
        shader.setMat4("uniProjMatrix", camera.GetPerspectiveProjectionMatrix());
        
        // points are world coordinates, no need to transform
        shader.setMat4("uniModelMatrix", glm::mat4(1.0f));

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

    void update(Camera *camera) // Rigid does not move, thus do not update vertexes' data
    {
        // Update all the positions of nodes
        for (int i = 0; i < springCount; i++) {
            Node* node1 = springs[i]->node1;
            Node* node2 = springs[i]->node2;
            vboPos[i * 2] = node1->worldPosition;
            vboPos[i * 2 + 1] = node2->worldPosition;
            vboNor[i * 2] = node1->normal;
            vboNor[i * 2 + 1] = node2->normal;
        }

        shader.use();

        glBindVertexArray(vaoID);

        glBindBuffer(GL_ARRAY_BUFFER, vboIDs[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, springCount * 2 * sizeof(glm::vec3), vboPos);
        glBindBuffer(GL_ARRAY_BUFFER, vboIDs[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, springCount * 2 * sizeof(glm::vec3), vboNor);

        /** View Matrix : The camera **/
        shader.setMat4("uniViewMatrix", camera->GetViewMatrix());

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        /** Draw **/
        // glLineWidth(3);
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
        render.init(cloth, defaultColor);
    }

    void update(Camera *camera) { render.update(camera); }
};