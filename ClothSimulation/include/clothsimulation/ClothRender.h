#pragma once

Draw_Mode Cloth::drawMode = DRAW_FACES;

struct ClothRender // Texture & Lighting
{
    const Cloth* cloth;
    int nodeCount; // Number of all nodes in faces

    glm::vec3* vboPos; // Position
    glm::vec2* vboTex; // Texture
    glm::vec3* vboNor; // Normal

    GLuint vaoID;
    GLuint vboIDs[3];
    GLuint texID;

    GLint aPtrPos;
    GLint aPtrTex;
    GLint aPtrNor;

    Shader shader;

    ClothRender(Cloth* cloth)
    {
        nodeCount = (int)(cloth->faces.size());
        if (nodeCount <= 0)
        {
            std::cout << "ERROR::ClothRender : No node exists." << std::endl;
            exit(-1);
        }

        this->cloth = cloth;

        vboPos = new glm::vec3[nodeCount];
        vboTex = new glm::vec2[nodeCount];
        vboNor = new glm::vec3[nodeCount];
        for (int i = 0; i < nodeCount; i++)
        {
            Node* n = cloth->faces[i];
            vboPos[i] = n->position;
            vboTex[i] = n->texCoord; // Texture coord will only be set here
            vboNor[i] = n->normal;
        }

        /** Build shader **/
        shader = Shader("resources/Shaders/ClothVS.glsl", "resources/Shaders/ClothFS.glsl");
        std::cout << "Cloth Program ID: " << shader.ID << std::endl;

        // Generate ID of VAO and VBOs
        glGenVertexArrays(1, &vaoID);
        glGenBuffers(3, vboIDs);

        // Attribute pointers of VAO
        aPtrPos = 0;
        aPtrTex = 1;
        aPtrNor = 2;
        // Bind VAO
        glBindVertexArray(vaoID);

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
        unsigned char* data = stbi_load("resources/Textures/fiber2.jpg", &texW, &texH, &colorChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texW, texH, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            // Automatically generate all the required mipmaps for the currently bound texture.
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else {
            cout << "Failed to load texture" << endl;
        }
        // Always free image memory
        stbi_image_free(data);

        /** Set uniform **/
        shader.use(); // Active shader before set uniform
        // Set texture sampler
        shader.setInt("uniTex", 0);

        /** Set Matrix **/
        glm::mat4 uniModelMatrix = cloth->GetModelMatrix();
        shader.setMat4("uniProjMatrix", camera.GetPerspectiveProjectionMatrix());
        shader.setMat4("uniModelMatrix", uniModelMatrix);

        /** Set Light **/
        shader.setVec3("uniLightPos", lightPos);
        shader.setVec3("uniLightColor", lightColor);

        // Cleanup
        glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbined VBO
        glBindVertexArray(0); // Unbined VAO
    }

    void destroy()
    {
        delete[] vboPos;
        delete[] vboTex;
        delete[] vboNor;

        if (vaoID)
        {
            glDeleteVertexArrays(1, &vaoID);
            glDeleteBuffers(3, vboIDs);
            vaoID = 0;
        }
        if (shader.ID)
        {
            glDeleteProgram(shader.ID);
            shader.ID = 0;
        }
    }

    void flush(Camera *camera)
    {
        // Update all the positions of nodes
        for (int i = 0; i < nodeCount; i++)
        { // Tex coordinate dose not change
            Node* n = cloth->faces[i];
            vboPos[i] = n->position;
            vboNor[i] = n->normal;
        }

        shader.use();

        glBindVertexArray(vaoID);

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
        glm::mat4 uniModelMatrix = cloth->GetModelMatrix();
        shader.setMat4("uniViewMatrix", camera->GetViewMatrix());
        shader.setMat4("uniModelMatrix", uniModelMatrix);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        /** Draw **/
        switch (Cloth::drawMode)
        {
        case DRAW_NODES:
            glDrawArrays(GL_POINTS, 0, nodeCount);
            break;
        case DRAW_LINES:
            glDrawArrays(GL_LINES, 0, nodeCount);
            break;
        default:
            glDrawArrays(GL_TRIANGLES, 0, nodeCount);
            break;
        }

        // End flushing
        glDisable(GL_BLEND);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glUseProgram(0);
    }
};
