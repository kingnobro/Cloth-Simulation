#pragma once

#include <vector>

#include "Spring.h"
#include "ModelRender.h"


// Default Cloth Values
const int NODE_DENSITY = 4;
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
    int collisionCount;             // 碰撞检测的迭代次数
    int clothID;                    // 布料的唯一标识符
    int width;
    int height;
    bool isSewed;                   // 是否处于缝合状态

    std::vector<Node*> nodes;       // 质点
    std::vector<Node*> sewNode;	    // 即将被缝合的顶点
    std::vector<size_t> faces;      // 构成面的顶点索引
    std::vector<Spring*> springs;   // 点之间的弹簧

    Cloth(string const &path, glm::vec3 position)
    {
        clothPos = position;
        clothID = ++clothNumber;
        isSewed = false;
        collisionCount = 0;

        readClothData(path);
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

            // 更新弹簧受力
            for (Spring* s : springs) {
                s->computeInternalForce(timeStep);
            }
            // 更新质点位置
            for (Node* n : nodes) {
                n->integrate(timeStep);
            }
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

private:
    void readClothData(string const& path)
    {
        const string vertexFile = path + ".vertex";
        const string indexFile = path + ".index";
        const string structuralFile = path + ".structural";
        const string shearFile = path + ".shear";
        const string bendingFile = path + ".bending";

        int count;
        int index1, index2, index3;
        float pos_x, pos_y, pos_z, tex_x, tex_y;
        float minX = FLT_MAX, maxX = FLT_MIN, minY = FLT_MAX, maxY = FLT_MIN;

        // 读取顶点数据
        FILE* fin = fopen(vertexFile.c_str(), "r");
        if (fin == nullptr) {
            std::cout << "ERROR::READ CLOTH DATA:: Cannot open .vertex file!" << std::endl;
        }
        fscanf(fin, "%d", &count);
        printf("Init cloth with %d nodes\n", count);
        for (int i = 0; i < count; i += 1) {
            fscanf(fin, "%f, %f, %f, %f, %f\n", &pos_x, &pos_y, &pos_z, &tex_x, &tex_y);
            Node* node = new Node(glm::vec3(pos_x, pos_y, pos_z));
            node->texCoord = glm::vec2(tex_x, tex_y);
            node->lastWorldPosition = node->worldPosition = node->localPosition + clothPos;
            nodes.push_back(node);

            // 更新边界
            minX = pos_x < minX ? pos_x : minX;
            minY = pos_y < minY ? pos_y : minY;
            maxX = pos_x > maxX ? pos_x : maxX;
            maxY = pos_y > maxY ? pos_y : maxY;
        }
        fclose(fin);
        width = maxX - minX;
        height = maxY - minY;
        std::cout << "cloth width:" << width << " height: " << height << std::endl;

        // 读取索引数据
        fin = fopen(indexFile.c_str(), "r");
        if (fin == nullptr) {
            std::cout << "ERROR::READ CLOTH DATA:: Cannot open .index file!" << std::endl;
        }
        fscanf(fin, "%d\n", &count);
        for (int i = 0; i < count; i += 1) {
            fscanf(fin, "%d, %d, %d\n", &index1, &index2, &index3);
            faces.push_back(index1);
            faces.push_back(index2);
            faces.push_back(index3);
        }
        fclose(fin);

        // 读取 structural 弹簧数据
        fin = fopen(structuralFile.c_str(), "r");
        if (fin == nullptr) {
            std::cout << "ERROR::READ CLOTH DATA:: Cannot open .structural file!" << std::endl;
        }
        fscanf(fin, "%d\n", &count);
        for (int i = 0; i < count; i += 1) {
            fscanf(fin, "%d, %d\n", &index1, &index2);
            springs.push_back(new Spring(nodes[index1], nodes[index2], structuralCoef));
        }
        fclose(fin);

        // 读取 shear 弹簧数据
        fin = fopen(shearFile.c_str(), "r");
        if (fin == nullptr) {
            std::cout << "ERROR::READ CLOTH DATA:: Cannot open .shear file!" << std::endl;
        }
        fscanf(fin, "%d\n", &count);
        for (int i = 0; i < count; i += 1) {
            fscanf(fin, "%d, %d\n", &index1, &index2);
            springs.push_back(new Spring(nodes[index1], nodes[index2], shearCoef));
        }
        fclose(fin);

        // 读取 bending 弹簧数据
        fin = fopen(bendingFile.c_str(), "r");
        if (fin == nullptr) {
            std::cout << "ERROR::READ CLOTH DATA:: Cannot open .bending file!" << std::endl;
        }
        fscanf(fin, "%d\n", &count);
        for (int i = 0; i < count; i += 1) {
            fscanf(fin, "%d, %d\n", &index1, &index2);
            springs.push_back(new Spring(nodes[index1], nodes[index2], bendingCoef));
        }
        fclose(fin);


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
            n1 = nodes[faces[3 * i]];
            n2 = nodes[faces[3 * i + 1]];
            n3 = nodes[faces[3 * i + 2]];

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
