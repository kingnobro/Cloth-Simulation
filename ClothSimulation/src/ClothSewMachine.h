#pragma once

#include <glm/glm.hpp>
#include <assert.h>
#include "Cloth.h"
#include "Shader.h"

class ClothSewMachine
{
public:
    Cloth* cloth1;
    Cloth* cloth2;
    Camera* camera;

    GLuint VAO;
    GLuint VBO;
    Shader shader;

    std::vector<glm::vec3> vertices;    // ��Ƭ�ϵĶ���
    std::vector<Spring*> springs;		// ��ϵ��ĵ���
    const float sewCoef = 1000.0f;
    bool resetable;	    // ���� reset �����, VAO VBO �ᱻɾ��
                        // ��һ�� bool ������¼�Ƿ��ڿ��� reset ״̬, ������ɾ���ᱨ��

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
        Node* n1 = nullptr;
        Node* n2 = nullptr;
        const float threshold = 0.15f;
        // ���·�ϵ�֮��ĵ���
        for (Spring* s : springs) {
            n1 = s->node1;
            n2 = s->node2;
            // upgrade: (�򵥵�) �������� merge
            if (glm::distance(n1->worldPosition, n2->worldPosition) < threshold) {
                n1->worldPosition = n2->worldPosition;
                continue;
            }
            s->computeInternalForce(timeStep);
            n1->integrate(timeStep);
            n2->integrate(timeStep);
        }
    }

    /*
     * �����Ƭ
     * ����ֱ���޸� Cloth �� clothPos, ��Ϊ�޸� closhPos �ᵼ�� modelMatrix �ı�
     * ������Ч����, ��һ����Ⱦѭ����, ��Ƭ˲���ƶ�
     * �ڼ�����ϵ�������֮������ϵ���, ���ҽ����ɵĳ�ʼ��������Ϊ 0, ��һ����Ⱦѭ����, ������ͻụ��ǣ������
     * Ϊ�˱����Ϻ���֮��Ŀ�϶, ��������ľ���С��ĳ�� threshold ʱ, ������ merge Ϊһ����
     */
    void SewCloths()
    {
        // ��Ϻ����Ƭ�����ٴη��
        if (cloth1 == nullptr || cloth2 == nullptr || cloth1->sewed || cloth2->sewed) {
            return;
        }
        // ����Ƭ��Ƭ��Ҫ��ϵĵ�ȡ��
        const std::vector<Node*>& sewNode1 = cloth1->sewNode;
        const std::vector<Node*>& sewNode2 = cloth2->sewNode;
        assert(sewNode1.size() == sewNode2.size());

        for (size_t i = 0, sz = sewNode1.size(); i < sz; i++) {
            Node* n1 = sewNode1[i];
            Node* n2 = sewNode2[i];

            // ����ʽ��Ϸ���: �ڷ�ϵ�֮����ϵ���, ��������Ȼ����
            n1->isSewed = n2->isSewed = true;
            Spring* s = new Spring(n1, n2, sewCoef);
            s->restLength = 0.0f;	// �÷�ϵ㾡���ܿ���
            springs.push_back(s);
        }
        cloth1->sewed = cloth2->sewed = true;
    }

    /*
     * ������Ƭ�ķ����
     */
    void drawSewingLine(const glm::mat4& view)
    {
        if (cloth1 == nullptr || cloth2 == nullptr || cloth1->sewed || cloth2->sewed) {
            return;
        }
        // ��Ƭ��λ�û����, �����ߵ�λ��ҲҪ����
        setSewNode();
        // std::cout << vertices.size() << std::endl;

        shader.use();
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_DYNAMIC_DRAW);

        shader.setMat4("view", view);

        // draw
        glDrawArrays(GL_LINES, 0, vertices.size());

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
        // ������Ƭ�ľֲ�����
        if (cloth1) cloth1->reset();
        if (cloth2) cloth2->reset();
        cloth1 = cloth2 = nullptr;

        // VAO VBO ֻ��ɾ��һ��
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
     * ÿ����Ƭ��һ������ϵĶ�������
     * ������Ƭ�Ĵ���������еĶ�����Ҫһһ��Ӧ
     * ��Ƭ�Ķ���(��������)����� vertices ��, ���� vertices[i] �� vertices[i+1] ������� (i%2=0)
     */ 
    void setSewNode() {
        vertices.clear();
        const std::vector<Node*>& sewNode1 = cloth1->sewNode;
        const std::vector<Node*>& sewNode2 = cloth2->sewNode;
        // assert(sewNode1.size() == sewNode2.size());
        
        for (size_t i = 0, sz = min(sewNode1.size(), sewNode2.size()); i < sz; i++) {
            vertices.push_back(sewNode1[i]->worldPosition);
            vertices.push_back(sewNode2[i]->worldPosition);
        }
    }

    void initialization()
    {
        resetable = true;

        // ���ô���ϵĶ���, ���ڻ��Ʒ����
        setSewNode();

        // ���Ʒ���ߵ� Shader
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
        // ���ϵĶ�����������������, ���Բ���Ҫ���� model ����
        shader.use();
        shader.setMat4("view", camera->GetViewMatrix());
        shader.setMat4("projection", camera->GetPerspectiveProjectionMatrix());

        // Cleanup
        glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbined VBO
        glBindVertexArray(0);		      // Unbined VAO
    }
};