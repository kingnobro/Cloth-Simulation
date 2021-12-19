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

    std::vector<Node*> vertices;    // 衣片上的顶点, 两片衣片的顶点交错排列
    std::vector<glm::vec3> positions;   // 衣片顶点的位置, 用于绘制缝合线
    std::vector<Spring*> springs;		// 缝合点间的弹簧
    const float sewCoef = 1200.0f;
    const float threshold = 0.05f;
    bool resetable;	    // 经过 reset 处理后, VAO VBO 会被删除
                        // 用一个 bool 变量记录是否处于可以 reset 状态, 避免多次删除会报错

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
        // 更新缝合点之间的弹簧
        for (Spring* s : springs) {
            n1 = s->node1;
            n2 = s->node2;
            // upgrade: (简单地) 将两个点 merge
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
     * 缝合衣片
     * 不能直接修改 Cloth 的 clothPos, 因为修改 closhPos 会导致 modelMatrix 改变
     * 这样的效果是, 下一个渲染循环中, 衣片瞬间移动
     * 在即将缝合的两个点之间添加上弹簧, 并且将弹簧的初始长度设置为 0, 下一个渲染循环中, 两个点就会互相牵引靠近
     * 为了避免缝合后布料之间的空隙, 当两个点的距离小于某个 threshold 时, 将它们 merge 为一个点
     */
    void SewCloths()
    {
        // 缝合后的衣片不能再次缝合
        if (cloth1 == nullptr || cloth2 == nullptr || cloth1->isSewed || cloth2->isSewed) {
            return;
        }
        // 把两片衣片中要缝合的点取出
        Node* n1, * n2;
        for (int i = 0; i < vertices.size(); i += 2) {
            n1 = vertices[i];
            n2 = vertices[i + 1];
            n1->isSewed = n2->isSewed = true;
            // 启发式缝合方法: 在缝合点之间加上弹簧, 让它们自然靠近
            Spring* s = new Spring(n1, n2, sewCoef);
            s->restLength = 0.001f;	// 让缝合点尽可能靠近
            springs.push_back(s);
        }
        cloth1->isSewed = cloth2->isSewed = true;
    }

    /*
     * 绘制衣片的缝合线
     */
    void drawSewingLine(const glm::mat4& view, const glm::mat4& projection)
    {
        if (cloth1 == nullptr || cloth2 == nullptr || cloth1->isSewed || cloth2->isSewed) {
            return;
        }
        // 衣片的位置会更新, 所以线的位置也要更新
        setSewNode();
        // std::cout << positions.size() << std::endl;

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
        // 重置衣片的局部坐标
        if (cloth1) cloth1->reset();
        if (cloth2) cloth2->reset();
        cloth1 = cloth2 = nullptr;

        // VAO VBO 只能删除一次
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
     * 每个衣片有一个待缝合的顶点数组
     * 两个衣片的待缝合数组中的顶点需要一一对应
     * 衣片的顶点(世界坐标)存放在 vertices 中, 并且 vertices[i] 和 vertices[i+1] 即将缝合 (i%2=0)
     */ 
    void setSewNode() {
        vertices.clear();
        positions.clear();

        // 把两片衣片中要缝合的点取出
        const std::vector<std::vector<Node*>>& sewNode1 = cloth1->sewNode;
        const std::vector<std::vector<Node*>>& sewNode2 = cloth2->sewNode;
        // assert(sewNode1.size() == sewNode2.size());

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

        // 设置待缝合的顶点, 用于绘制缝合线
        setSewNode();

        // 绘制缝合线的 Shader
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
        // 布料的顶点坐标是世界坐标, 所以不需要传入 model 矩阵
        shader.use();
        shader.setMat4("view", camera->GetViewMatrix());
        shader.setMat4("projection", camera->GetPerspectiveProjectionMatrix());

        // Cleanup
        glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbined VBO
        glBindVertexArray(0);		      // Unbined VAO
    }
};

#endif