﻿#pragma once

#include <vector>

#include "Spring.h"
#include "ModelRender.h"

extern const int iterationFreq;
extern glm::vec3 gravity;

// Default Cloth Values
const int NODE_DENSITY = 4;
const float STRUCTURAL_COEF = 400.0;
const float SHEAR_COEF = 80.0;
const float BENDING_COEF = 50.0;
const int MAX_COLLISION_TIME = 2000;

enum Draw_Mode
{
    DRAW_NODES,
    DRAW_LINES,
    DRAW_FACES
};

class Cloth
{
public:
    static Draw_Mode drawMode;

    const int nodesDensity = NODE_DENSITY;
    const float structuralCoef = STRUCTURAL_COEF;
    const float shearCoef = SHEAR_COEF;
    const float bendingCoef = BENDING_COEF;
    int width;
    int height;
    int nodesPerRow;
    int nodesPerCol;
    int collisionCount;
    glm::vec3 clothPos;
    bool sewed;

    std::vector<Node*> nodes;
    std::vector<Node*> faces;
    std::vector<Node*> sewNode;	// 即将被缝合的顶点
    std::vector<Spring*> springs;

    Cloth(glm::vec3 pos, glm::vec2 size, int ID)
    {
        clothPos = pos;
        width = (int)size.x;
        height = (int)size.y;
        clothID = ID;
        sewed = false;
        collisionCount = 0;

        init();
    }

    ~Cloth()
    {
        for (int i = 0; i < nodes.size(); i++)
        {
            delete nodes[i];
        }
        for (int i = 0; i < springs.size(); i++)
        {
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

            // 更新弹簧受力
            for (Spring* s : springs) {
                s->computeInternalForce(timeStep);
            }
            // 更新质点位置
            for (Node* n : nodes) {
                n->integrate(timeStep);
            }
            // 碰撞检测与碰撞响应. 开始缝制之后才需要检测碰撞
            if (sewed) {
                for (Node* node : nodes) {
                    if (modelRender.collideWithModel(node)) {
                        modelRender.collisionResponse(node);
                    }
                }
                collisionCount += 1;
            }
        }
        computeFaceNormal();
    }

    /*
     * 移动衣片位置. 衣片坐标和质点的世界坐标需要改变 
     */
    void moveCloth(glm::vec3 offset)
    {
        clothPos += offset;
        for (Node* n : nodes) {
            n->worldPosition += offset;
            n->lastWorldPosition = n->worldPosition;
        }
    }

    int GetClothID() const
    {
        return clothID;
    }

    Node* getNode(int x, int y)
    {
        if (x >= 0 && x < nodesPerRow && y >= 0 && y < nodesPerCol) {
            return nodes[y * nodesPerRow + x];
        }
        return nullptr;
    }

    void reset()
    {
        // 重置局部坐标
        for (Node* n : nodes) {
            n->lastWorldPosition = n->worldPosition = clothPos + n->localPosition;
            n->reset();
        }
        // 恢复到未缝合的状态
        sewed = false;
        sewNode.clear();
        collisionCount = 0;
    }

private:
    int clothID; 

    void init()
    {
        nodesPerRow = width * nodesDensity;
        nodesPerCol = height * nodesDensity;

        /** Add Nodes **/
        printf("Init cloth with %d nodes\n", nodesPerRow * nodesPerCol);
        for (int y = 0; y < nodesPerCol; y++) {
            for (int x = 0; x < nodesPerRow; x++) {
                float pos_x = (float)x / nodesDensity;
                float pos_y = -((float)y / nodesDensity);   // 衣物原点在左上角, 所以衣服上的点 y 值是负的
                float pos_z = 0;
                float tex_x = (float)x / (nodesPerRow - 1);
                float tex_y = (float)y / (1 - nodesPerCol);
                Node* node = new Node(glm::vec3(pos_x, pos_y, pos_z));
                node->lastWorldPosition = node->worldPosition = node->localPosition + clothPos;
                node->texCoord = glm::vec2(tex_x, tex_y);
                nodes.push_back(node);
            }
        }

        /** Add springs **/
        for (int x = 0; x < nodesPerRow; x++) {
            for (int y = 0; y < nodesPerCol; y++) {
                /** Structural **/
                if (x < nodesPerRow - 1) springs.push_back(new Spring(getNode(x, y), getNode(x + 1, y), structuralCoef));
                if (y < nodesPerCol - 1) springs.push_back(new Spring(getNode(x, y), getNode(x, y + 1), structuralCoef));
                /** Shear **/
                if (x < nodesPerRow - 1 && y < nodesPerCol - 1) {
                    springs.push_back(new Spring(getNode(x, y), getNode(x + 1, y + 1), shearCoef));
                    springs.push_back(new Spring(getNode(x + 1, y), getNode(x, y + 1), shearCoef));
                }
                /** Bending **/
                if (x < nodesPerRow - 2) springs.push_back(new Spring(getNode(x, y), getNode(x + 2, y), bendingCoef));
                if (y < nodesPerCol - 2) springs.push_back(new Spring(getNode(x, y), getNode(x, y + 2), bendingCoef));
            }
        }

        /** Triangle faces **/
        for (int x = 0; x < nodesPerRow - 1; x++) {
            for (int y = 0; y < nodesPerCol - 1; y++) {
                // Left upper triangle
                faces.push_back(getNode(x + 1, y));
                faces.push_back(getNode(x, y));
                faces.push_back(getNode(x, y + 1));
                // Right bottom triangle
                faces.push_back(getNode(x + 1, y + 1));
                faces.push_back(getNode(x + 1, y));
                faces.push_back(getNode(x, y + 1));
            }
        }
    }

    /*
     * 计算面的法向量, 用于生成光照效果
     */
    void computeFaceNormal()
    {
        /** Reset nodes' normal **/
        glm::vec3 normal(0.0f);
        for (Node* node : nodes) {
            node->normal = normal;
        }
        /** Compute normal of each face **/
        Node* n1;
        Node* n2;
        Node* n3;
        for (size_t i = 0; i < faces.size() / 3; i++) { // 3 nodes in each face
            n1 = faces[3 * i];
            n2 = faces[3 * i + 1];
            n3 = faces[3 * i + 2];

            // Face normal
            normal = glm::cross(n2->worldPosition - n1->worldPosition, n3->worldPosition - n1->worldPosition);
            // Add all face normal
            n1->normal += normal;
            n2->normal += normal;
            n3->normal += normal;
        }

        for (Node* node : nodes) {
            node->normal = glm::normalize(node->normal);
        }
    }
};
