#ifndef CLOTH_H
#define CLOTH_H

#include <vector>

#include "Spring.h"
#include "ModelRender.h"

// Default Cloth Values
const float STRUCTURAL_COEF = 40.0;
const float SHEAR_COEF = 80.0;
const float BENDING_COEF = 50.0;
const float SCALE_COEF = 0.01;
const int MAX_COLLISION_TIME = 2000;

// unique identifier of cloth
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
    static Draw_Mode drawMode;

    const float structuralCoef = STRUCTURAL_COEF;
    const float shearCoef = SHEAR_COEF;
    const float bendingCoef = BENDING_COEF;
    const float scaleCoef = SCALE_COEF;

    glm::vec3 clothPos;             // world space of cloth
    glm::vec3 leftUpper;            // corners of bounding box
    glm::vec3 rightUpper;
    glm::vec3 rightBottom;
    glm::mat4 modelMatrix;

    int collisionCount;
    int clothID;
    int width;
    int height;
    bool isSewed;                   // whether the cloth is sewed

    std::vector<Node*> nodes;
    std::vector<Node*> sewNode;	    // nodes to be sewed
    std::vector<Node*> faces;       // every 3 nodes make up a face; use to draw triangles
    std::vector<Spring*> springs;   // springs of cloth

    Cloth(glm::vec3 position, float minX, float maxX, float minY, float maxY)
    {
        clothPos = position;
        width = int(maxX - minX);
        height = int(maxY - minY);
        clothID = ++clothNumber;
        isSewed = false;
        collisionCount = 0;

        modelMatrix = glm::translate(glm::mat4(1.0f), clothPos);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(scaleCoef, scaleCoef, scaleCoef));
        leftUpper = modelMatrix * glm::vec4(clothPos + glm::vec3(minX, maxY, 0.0f), 1.0f);
        rightUpper = modelMatrix * glm::vec4(clothPos + glm::vec3(maxX, maxY, 0.0f), 1.0f);
        rightBottom = modelMatrix * glm::vec4(clothPos + glm::vec3(maxX, minY, 0.0f), 1.0f);
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
        // no need to update positions after sewing
        if (collisionCount > MAX_COLLISION_TIME) {
            return;
        }

        for (Spring* s : springs) {
            s->computeInternalForce(timeStep);
        }
        for (Node* n : nodes) {
            n->integrate(timeStep);
        }
        // collision detection and response
        if (isSewed) {
            for (Node* node : nodes) {
                if (modelRender.collideWithModel(node)) {
                    modelRender.collisionResponse(node);
                }
            }
            collisionCount += 1;
        }
        // todo: uncomment me
        //computeFaceNormal();
    }

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
        // reset position
        for (Node* n : nodes) {
            n->lastWorldPosition = n->worldPosition = modelMatrix * glm::vec4(clothPos + n->localPosition, 1.0f);
            n->reset();
        }
        isSewed = false;
        sewNode.clear();
        collisionCount = 0;
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

#endif