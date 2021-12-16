#pragma once

#include <vector>

#include "Spring.h"
#include "ModelRender.h"

// Default Cloth Values
const float STRUCTURAL_COEF = 400.0;
const float SHEAR_COEF = 80.0;
const float BENDING_COEF = 50.0;
const int MAX_COLLISION_TIME = 2000;
const int iterationFreq = 10;   // todo: remove me. 

// 用于唯一标识一块布料
int clothNumber = 0;

enum Draw_Mode
{
    DRAW_NODES,
    DRAW_LINES,
    DRAW_FACES
};

class Cloth
{
public:
    static Draw_Mode drawMode;      // 显示方式, 可以为 点、线、面

    const float structuralCoef = STRUCTURAL_COEF;
    const float shearCoef = SHEAR_COEF;
    const float bendingCoef = BENDING_COEF;

    glm::vec3 clothPos;             // 能够包围住衣片的矩形的左上角的世界坐标
    glm::vec3 leftUpper, rightUpper, rightBottom;
    int collisionCount;             // 碰撞检测的迭代次数
    int clothID;
    int width;
    int height;
    bool isSewed;                   // 是否处于缝合状态

    std::vector<Node*> nodes;       // 质点(无序的)
    std::vector<Node*> sewNode;	    // 即将被缝合的顶点
    std::vector<Node*> faces;       // 构成面的顶点, OpenGL画图时用的是faces中的数据(因为GL_TRIANGLE要求三角形顶点有序）
    std::vector<Spring*> springs;   // 点之间的弹簧

    Cloth(glm::vec3 position, float minX, float maxX, float minY, float maxY)
    {
        clothPos = position;
        leftUpper = clothPos + glm::vec3(minX, maxY, 0.0f);
        rightUpper = clothPos + glm::vec3(maxX, maxY, 0.0f);
        rightBottom = clothPos + glm::vec3(maxX, minY, 0.0f);
        clothID = ++clothNumber;
        isSewed = false;
        collisionCount = 0;
    }

    ~Cloth()
    {
        for (int i = 0; i < nodes.size(); i++) {
            delete nodes[i];
        }
        for (int i = 0; i < springs.size(); i++) {
            delete springs[i];
        }
        nodes.clear();
        springs.clear();
        faces.clear();
    }
    
    static void modifyDrawMode(Draw_Mode mode)
    {
        drawMode = mode;
    }

    /*
     * update force, movement, collision and normals in every render loop
     */
    void update(float timeStep, ModelRender& modelRender)
    {
        for (int i = 0; i < iterationFreq; i++)
        {
            // 此时衣片已经缝合在衣服上了, 不需要再更新弹簧受力和质点位置了
            if (collisionCount > MAX_COLLISION_TIME) {
                continue;
            }

            // TODO: update force and positions
            // 更新弹簧受力
            // for (Spring* s : springs) {
            //     s->computeInternalForce(timeStep);
            // }
            // 更新质点位置
            // for (Node* n : nodes) {
            //     n->integrate(timeStep);
            // }
            // 碰撞检测与碰撞响应. 开始缝制  之后才需要检测碰撞
            if (isSewed) {
                for (Node* node : nodes) {
                    if (modelRender.collideWithModel(node)) {
                        modelRender.collisionResponse(node);
                    }
                }
                collisionCount += 1;
            }
        }
        // todo: uncomment me
        //computeFaceNormal();
    }

    /*
     * 移动衣片位置. 衣片坐标和质点的世界坐标需要改变 
     */
    void moveCloth(glm::vec3 offset)
    {
        clothPos += offset;
        leftUpper += offset;
        rightUpper += offset;
        rightBottom += offset;
        for (Node* n : nodes) {
            n->worldPosition += offset;
            n->lastWorldPosition = n->worldPosition;
        }
    }

    int GetClothID() const
    {
        return clothID;
    }

    void reset()
    {
        // 重置局部坐标
        for (Node* n : nodes) {
            n->lastWorldPosition = n->worldPosition = clothPos + n->localPosition;
            n->reset();
        }
        // 恢复到未缝合的状态
        isSewed = false;
        sewNode.clear();
        collisionCount = 0;
    }

    glm::mat4 GetModelMatrix() const {
        float scaleFactor = 0.01f;
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, clothPos);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(scaleFactor, scaleFactor, scaleFactor));
        return modelMatrix;
    }

private:

    /** Add springs **/
    // /** Structural **/
    // if (x < nodesPerRow - 1) springs.push_back(new Spring(getNode(x, y), getNode(x + 1, y), structuralCoef));
    // if (y < nodesPerCol - 1) springs.push_back(new Spring(getNode(x, y), getNode(x, y + 1), structuralCoef));
    // /** Shear **/
    // if (x < nodesPerRow - 1 && y < nodesPerCol - 1) {
    //     springs.push_back(new Spring(getNode(x, y), getNode(x + 1, y + 1), shearCoef));
    //     springs.push_back(new Spring(getNode(x + 1, y), getNode(x, y + 1), shearCoef));
    // }
    // /** Bending **/
    // if (x < nodesPerRow - 2) springs.push_back(new Spring(getNode(x, y), getNode(x + 2, y), bendingCoef));
    // if (y < nodesPerCol - 2) springs.push_back(new Spring(getNode(x, y), getNode(x, y + 2), bendingCoef));

    /*
     * 计算面的法向量, 用于生成光照效果
     */
    //void computeFaceNormal()
    //{
    //    /** Reset nodes' normal **/
    //    glm::vec3 normal(0.0f);
    //    for (Node* node : nodes) {
    //        node->normal = normal;
    //    }
    //    /** Compute normal of each face **/
    //    Node* n1;
    //    Node* n2;
    //    Node* n3;
    //    for (size_t i = 0; i < faces.size() / 3; i++) { // 3 nodes in each face
    //        n1 = faces[3 * i];
    //        n2 = faces[3 * i + 1];
    //        n3 = faces[3 * i + 2];

    //        // Face normal
    //        normal = glm::cross(n2->worldPosition - n1->worldPosition, n3->worldPosition - n1->worldPosition);
    //        // Add all face normal
    //        n1->normal += normal;
    //        n2->normal += normal;
    //        n3->normal += normal;
    //    }

    //    for (Node* node : nodes) {
    //        node->normal = glm::normalize(node->normal);
    //    }
    //}
};
