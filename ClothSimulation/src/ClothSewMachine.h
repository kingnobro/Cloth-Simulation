#ifndef CLOTH_SEWMACHINE_H
#define CLOTH_SEWMACHINE_H
#include <assert.h>
#include "Cloth.h"

class ClothSewMachine
{
public:
    Cloth* cloth1;
    Cloth* cloth2;
    Camera* camera;

    GLuint VAO;
    GLuint VBO;
    Shader shader;

    std::vector<Node*> vertices;        // nodes to be sewed
    std::vector<glm::vec3> positions;   // position of vertices for drawing sewing lines
    std::vector<Spring*> springs;		// springs between nodes to be sewed
    const float sewCoef = 1200.0f;
    const float threshold = 0.05f;
    bool resetable;	    // after reset(), VAO VBO will be deleted
                        // therefore 'resetable' is used to decide whether we can reset()

    ClothSewMachine(Camera* cam)
    {
        cloth1 = cloth2 = nullptr;
        camera = cam;
        resetable = false;
    }

    ~ClothSewMachine()
    {
        if (resetable)
        {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteProgram(shader.ID);
            for (Spring* s : springs) {
                delete s;
            }
            springs.clear();
        }
    }

    void update(float timeStep)
    {
        if (!cloth1 || !cloth2 || !cloth1->isSewed || !cloth2->isSewed) {
            return;
        }
        Node* n1 = nullptr;
        Node* n2 = nullptr;
        // update springs between nodes to be sewed
        for (Spring* s : springs) {
            n1 = s->node1;
            n2 = s->node2;
            // upgrade: now we just simply move nodes to middle point
            if (glm::distance(n1->worldPosition, n2->worldPosition) < threshold) {
                glm::vec3 newPos = (n1->worldPosition + n2->worldPosition) / 2.0f;
                n1->worldPosition = n2->worldPosition = newPos;
                continue;
            }
            s->computeInternalForce(timeStep);
            n1->integrate(timeStep);
            n2->integrate(timeStep);
        }
    }

    /*
     * Sew Cloth in Heuristic method: we add springs between nodes, therefore springs can drag them together
     */
    void SewCloths()
    {
        // cloths that have been sewed should not be sewed again
        if (cloth1 == nullptr || cloth2 == nullptr || cloth1->isSewed || cloth2->isSewed) {
            return;
        }
        
        Node* n1, * n2;
        for (int i = 0; i < vertices.size(); i += 2) {
            n1 = vertices[i];
            n2 = vertices[i + 1];
            n1->isSewed = n2->isSewed = true;
            // Heuristic method
            Spring* s = new Spring(n1, n2, sewCoef);
            s->restLength = 0.001f;	// small rest length to make cloths closer
            springs.push_back(s);
        }
        cloth1->isSewed = cloth2->isSewed = true;
    }

    void drawSewingLine(const glm::mat4& view, const glm::mat4& projection)
    {
        if (cloth1 == nullptr || cloth2 == nullptr || cloth1->isSewed || cloth2->isSewed) {
            return;
        }
        // position of nodes may be updated, so we need to set them
        setSewNode();

        shader.use();
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3), positions.data(), GL_DYNAMIC_DRAW);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        // draw
        glDrawArrays(GL_LINES, 0, positions.size());

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glUseProgram(0);
    }

    void setCandidateCloth(Cloth* cloth)
    {
        if (cloth == nullptr)
        {
            return;
        }
        if (cloth1 == nullptr)
        {
            cloth1 = cloth;
        }
        else if (cloth1 != cloth && cloth2 == nullptr)
        {
            cloth2 = cloth;
            initialization();
        }
        else if (cloth1 != cloth && cloth2 != cloth) {
            cloth1 = cloth;
            cloth2 = nullptr;
        }
    }

    void reset()
    {
        if (cloth1) cloth1->reset();
        if (cloth2) cloth2->reset();
        cloth1 = cloth2 = nullptr;

        // VAO VBO can only be deleted once
        if (resetable)
        {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteProgram(shader.ID);
            for (Spring* s : springs) {
                delete s;
            }
            springs.clear();
        }
        resetable = false;
    }


private:
    /* 
     * every Cloth has a vector SewNode, which stores nodes to be sewed later
     * sewNodes in two cloths should be matched
     * we put data in sewNode into 'vertices', plus vertices[i] and vertices[i+1] will be sewed (i % 2 = 0)
     */ 
    void setSewNode() {
        vertices.clear();
        positions.clear();

        // retrieve data
        const std::vector<std::vector<Node*>>& sewNode1 = cloth1->sewNode;
        const std::vector<std::vector<Node*>>& sewNode2 = cloth2->sewNode;

        for (size_t i = 0, seg_sz = std::min(sewNode1.size(), sewNode2.size()); i < seg_sz; i++) {
            const std::vector<Node*>& nodes1 = sewNode1[i];
            const std::vector<Node*>& nodes2 = sewNode2[i];

            int j = 0;
            for (size_t cnt = 0, sz1 = nodes1.size(), sz2 = nodes2.size(), node_sz = std::min(sz1, sz2); cnt < node_sz; cnt++) {
                Node* n1, * n2;
                // number of nodes on contour may not be identical, we need to distribute them evenly
                if (cnt % 2 == 0) {
                    n1 = nodes1[j];
                    n2 = nodes2[j];
                }
                else {
                    n1 = nodes1[sz1 - 1 - j];
                    n2 = nodes2[sz2 - 1 - j];
                    j += 1;
                }
                vertices.push_back(n1);
                vertices.push_back(n2);
                positions.push_back(n1->worldPosition);
                positions.push_back(n2->worldPosition);
            }
        }
    }

    void initialization()
    {
        resetable = true;

        // set nodes to be sewed
        setSewNode();

        shader = Shader("src/shaders/LineVS.glsl", "src/shaders/LineFS.glsl");
        std::cout << "Sew Program ID: " << shader.ID << std::endl;

        // generate ID of VAO and VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        // Bind VAO
        glBindVertexArray(VAO);

        // position buffer
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_DYNAMIC_DRAW);

        // enalbe attribute pointers
        glEnableVertexAttribArray(0);

        // set Uniforms
        // no need to set model matrix, since coord is already world position
        shader.use();
        shader.setMat4("view", camera->GetViewMatrix());
        shader.setMat4("projection", camera->GetPerspectiveProjectionMatrix());

        // Cleanup
        glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbined VBO
        glBindVertexArray(0);		      // Unbined VAO
    }
};

#endif