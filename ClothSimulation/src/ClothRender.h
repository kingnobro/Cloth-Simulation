#ifndef CLOTH_RENDER_H
#define CLOTH_RENDER_H

#include "Cloth.h"

Draw_Mode Cloth::drawMode = DRAW_FACES;

struct ClothRender // Texture & Lighting
{
    const Cloth* cloth;
    int nodeCount; // Number of nodes in cloth.faces
    int contourSize;    // Number of nodes in cloth.contour

    glm::vec3* vboPos; // Position
    glm::vec2* vboTex; // Texture
    glm::vec3* vboNor; // Normal
    glm::vec3* vboSegmentPos;

    GLuint vaoIDs[2]; // 1 for cloth, 1 for segment
    GLuint vboIDs[4]; // 3 for cloth, 1 for segment
    GLuint texID;

    GLint aPtrPos;
    GLint aPtrTex;
    GLint aPtrNor;

    Shader clothShader;
    Shader segmentShader;   // shader of selected segments on cloth's contour

    /*
     * 设置 shader VAO VBO 
     */
    ClothRender(Cloth* cloth)
    {
        nodeCount = (int)(cloth->faces.size());
        contourSize = (int)(cloth->contour.size());
        if (nodeCount <= 0)
        {
            std::cout << "ERROR::ClothRender : No node exists." << std::endl;
            exit(-1);
        }
        if (contourSize <= 0)
        {
            std::cout << "ERROR::ClothRender : No contour exists." << std::endl;
            exit(-1);
        }

        this->cloth = cloth;

        vboPos = new glm::vec3[nodeCount];
        vboTex = new glm::vec2[nodeCount];
        vboNor = new glm::vec3[nodeCount];
        vboSegmentPos = new glm::vec3[2 * contourSize]; // n nodes on contour will generate n lines at most, each line have 2 nodes
        for (int i = 0; i < nodeCount; i++)
        {
            Node* n = cloth->faces[i];
            vboPos[i] = n->worldPosition;
            vboTex[i] = n->texCoord; // Texture coord will only be set here
            vboNor[i] = n->normal;
        }

        /** Build shader **/
        clothShader = Shader("src/shaders/ClothVS.glsl", "src/shaders/ClothFS.glsl");
        segmentShader = Shader("src/shaders/LineVS.glsl", "src/shaders/LineFS.glsl");
        std::cout << "Cloth Program ID: " << clothShader.ID << std::endl;
        std::cout << "Segment Program ID: " << segmentShader.ID << std::endl;

        // Generate ID of VAO and VBOs
        glGenVertexArrays(2, vaoIDs);
        glGenBuffers(4, vboIDs);

        // Attribute pointers of VAO
        aPtrPos = 0;
        aPtrTex = 1;
        aPtrNor = 2;
        // Bind VAO
        glBindVertexArray(vaoIDs[0]);

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
        unsigned char* data = stbi_load("assets/textures/white.jpg", &texW, &texH, &colorChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texW, texH, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            // Automatically generate all the required mipmaps for the currently bound texture.
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else {
            std::cout << "Failed to load texture" << std::endl;
        }
        // Always free image memory
        stbi_image_free(data);

        /** Set uniform **/
        clothShader.use(); // Active shader before set uniform
        // Set texture sampler
        clothShader.setInt("uniTex", 0);
        // points are already world coordinates, no need to transform
        clothShader.setMat4("uniProjMatrix", camera.GetPerspectiveProjectionMatrix());
        clothShader.setMat4("uniModelMatrix", glm::mat4(1.0f));
        clothShader.setVec3("uniLightPos", lightPos);
        clothShader.setVec3("uniLightColor", lightColor);

        // Set Segment Data
        glBindVertexArray(vaoIDs[1]);
        // Contour Segment buffer
        glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);
        segmentShader.use();
        segmentShader.setMat4("projection", camera.GetPerspectiveProjectionMatrix());
        segmentShader.setMat4("view", camera.GetViewMatrix());

        // Cleanup
        glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbined VBO
        glBindVertexArray(0); // Unbined VAO
    }

    void destroy()
    {
        delete[] vboPos;
        delete[] vboTex;
        delete[] vboNor;
        delete[] vboSegmentPos;

        for (int i = 0; i < 2; i++) {
            if (vaoIDs[i])
            {
                glDeleteVertexArrays(1, &vaoIDs[i]);
                glDeleteBuffers(3, vboIDs);
                vaoIDs[i] = 0;
            }
        }
        if (clothShader.ID)
        {
            glDeleteProgram(clothShader.ID);
            clothShader.ID = 0;
        }
        if (segmentShader.ID)
        {
            glDeleteProgram(segmentShader.ID);
            segmentShader.ID = 0;
        }
    }

    /*
     * 更新数据 
     */
    void update(Camera *camera)
    {
        // Update all the positions of nodes
        for (int i = 0; i < nodeCount; i++)
        { // Tex coordinate dose not change
            Node* n = cloth->faces[i];
            vboPos[i] = n->worldPosition;
            vboNor[i] = n->normal;
        }

        clothShader.use();

        glBindVertexArray(vaoIDs[0]);

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
        clothShader.setMat4("uniViewMatrix", camera->GetViewMatrix());
        clothShader.setMat4("uniProjMatrix", camera->GetPerspectiveProjectionMatrix());
        // points are already world coordinates, no need to transform
        clothShader.setMat4("uniModelMatrix", glm::mat4(1.0f));

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        /** Draw **/
        switch (Cloth::drawMode)
        {
        case DRAW_NODES:
            glDrawArrays(GL_POINTS, 0, nodeCount);
            break;
        case DRAW_FACES:
            glDrawArrays(GL_TRIANGLES, 0, nodeCount);
            break;
        }

		// draw segments
        if (!cloth->isSewed) {
            segmentShader.use();
            glBindVertexArray(vaoIDs[1]);
            glLineWidth(5);
            for (int i = 0, seg_sz = cloth->sewNode.size(); i < seg_sz; i++) {
                const std::vector<Node*>& seg = cloth->sewNode[i];
                int lineCount = 2 * (seg.size() - 1);
                for (int j = 0, node_sz = seg.size(); j < node_sz - 1; j++) {
                    vboSegmentPos[j * 2] = seg[j]->worldPosition;
                    vboSegmentPos[j * 2 + 1] = seg[j + 1]->worldPosition;
                }
                glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
                glBufferData(GL_ARRAY_BUFFER, lineCount * sizeof(glm::vec3), vboSegmentPos, GL_DYNAMIC_DRAW);
                segmentShader.setMat4("view", camera->GetViewMatrix());
                segmentShader.setMat4("projection", camera->GetPerspectiveProjectionMatrix());
                glDrawArrays(GL_LINES, 0, lineCount);
            }
            glLineWidth(1);
        }

        // End flushing
        glDisable(GL_BLEND);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glUseProgram(0);
    }
};

#endif