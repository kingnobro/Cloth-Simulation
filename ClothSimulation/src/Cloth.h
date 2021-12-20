#ifndef CLOTH_H
#define CLOTH_H

#include <vector>

#include "Spring.h"
#include "ModelRender.h"

// Default Cloth Values
const float STRUCTURAL_COEF = 500.0;
const float SHEAR_COEF = 100.0;
const float BENDING_COEF = 200.0;
const float SCALE_COEF = 0.0105;
const int MAX_COLLISION_TIME = 700;

// unique identifier of cloth, used to select cloths
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

    glm::vec3 leftUpper;            // corners of the bounding box
    glm::vec3 rightUpper;
    glm::vec3 rightBottom;
    glm::mat4 modelMatrix;
    glm::mat4 invModelMatrix;

    int collisionCount;
    int clothID;
    int width;
    int height;
    bool isSewed;                   // whether the cloth is sewed

    std::vector<Node*> nodes;
    std::vector<Node*> faces;       // every 3 nodes make up a face; use to draw triangles
    std::vector<Node*> contour;
    std::vector<std::vector<Node*>> sewNode;	// nodes to be sewed
    std::vector<std::vector<Node*>> segments;
    std::vector<Spring*> springs;   // springs of cloth

    Cloth(glm::vec3 position, float minX, float maxX, float minY, float maxY)
    {
        width = int(maxX - minX);
        height = int(maxY - minY);
        clothID = ++clothNumber;
        isSewed = false;
        collisionCount = 0;

        modelMatrix = glm::translate(glm::mat4(1.0f), position);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(scaleCoef, scaleCoef, scaleCoef));
        invModelMatrix = glm::inverse(modelMatrix);

        leftUpper = modelMatrix * glm::vec4(minX, maxY, 0.0f, 1.0f);
        rightUpper = modelMatrix * glm::vec4(maxX, maxY, 0.0f, 1.0f);
        rightBottom = modelMatrix * glm::vec4(maxX, minY, 0.0f, 1.0f);
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
    void update(float timeStep)
    {
        computeFaceNormal();
        for (Spring* s : springs) {
            s->computeInternalForce(timeStep);
        }
        for (Node* n : nodes) {
            n->integrate(timeStep);
        }
    }

    /*
     * collision detection and response with model 
     */
    void modelCollision(ModelRender& modelRender) {
        for (Node* node : nodes) {
            if (modelRender.collideWithModel(node)) {
                modelRender.collisionResponse(node);
            }
        }
        collisionCount += 1;
    }

    /*
     * cloth self collision detection and response 
     */
    void clothCollision() {
        for (Node* n : nodes) {

        }
    }

    /*
     * @param: offset is under world coordinates
     */
    void moveCloth(glm::vec3 offset)
    {
        leftUpper += offset;
        rightUpper += offset;
        rightBottom += offset;
        glm::vec3 localOffset = invModelMatrix * glm::vec4(offset, 0.0f);

        for (Node* n : nodes) {
            n->worldPosition += offset;
            n->lastWorldPosition = n->worldPosition;

            // localPosition should be modified either
            // otherwise reset() will set cloths to original places, instead of positions before sewing
            n->localPosition += localOffset;
        }
    }

    int GetClothID() const
    {
        return clothID;
    }

    void reset()
    {
        // set position to that before sewing 
        for (Node* n : nodes) {
            n->lastWorldPosition = n->worldPosition = modelMatrix * glm::vec4(n->localPosition, 1.0f);
            n->reset();
        }
        isSewed = false;
        sewNode.clear();
        collisionCount = 0;
    }


private:
    /*
     * calculate face normals to generate lighting effects
     */
    void computeFaceNormal()
    {
        /** Reset nodes' normal **/
        glm::vec3 normal(0.0f);
        for (Node* node : nodes) {
            node->normal = normal;
        }
        /** Compute normal of each face **/
        Node* n1, * n2, * n3;
        assert(faces.size() % 3 == 0);

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

#endif