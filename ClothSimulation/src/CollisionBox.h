#ifndef COLLISION_BOX_H
#define BOLLISION_BOX_H

#include <float.h>

#include "Camera.h"

/*
 * ��Χ 3D ģ�͵���ײ��, ����ǰ�������������, �����������ͼ�ͷ���ͼ, ���ͼ������ײ���
 */
class CollisionBox
{
public:
    float minX, minY, minZ;
    float maxX, maxY, maxZ;

    float width;
    float height;
    float length;
    float phi;			// projection area of the orthographic camera, phi = max(height, length)
    int scr_width;      // ���ڻ�ȡ front position
    int scr_height;     // ���ڻ�ȡ back position
    glm::vec3 centroid; // ��Χ�е�����
    glm::vec3 origin;	// ��Χ�пռ������ԭ��, �ں�����������·�
    Camera frontCamera;	// Camera around AABB box for depth map generation
    Camera backCamera;

    CollisionBox()
    {
        minX = minY = minZ = FLT_MAX;
        maxX = maxY = maxZ = -FLT_MAX;
        width = height = length = phi = 0;
        centroid = glm::vec3(0.0f);
    }

    /*
     * update the boundary of AABB box
     */
    void updateBoundary(const glm::vec3& position)
    {
        float x = position.x;
        float y = position.y;
        float z = position.z;

        if (x < minX) minX = x;
        if (x > maxX) maxX = x;

        if (y < minY) minY = y;
        if (y > maxY) maxY = y;

        if (z < minZ) minZ = z;
        if (z > maxZ) maxZ = z;
    }

    /*
     * set arguments of the box 
     */
    void setBox()
    {
        // set width, height length of the AABB box
        width = maxZ - minZ;
        height = maxY - minY;
        length = maxX - minX;
        phi = std::max(length, height);
        centroid = glm::vec3((maxX + minX) / 2, (maxY + minY) / 2, (maxZ + minZ) / 2);
        origin = centroid - glm::vec3(0.0f, height / 2, width / 2);

        // update front and back camera
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 frontPosition = centroid + glm::vec3(0.0f, 0.0f, width / 2);
        glm::vec3 backPosition = centroid - glm::vec3(0.0f, 0.0f, width / 2);
        float frontYaw = -90.0f;
        float backYaw = 90.0f;
        frontCamera = Camera(Orthographic, frontPosition, up, frontYaw);
        backCamera = Camera(Orthographic, backPosition, up, backYaw);
        frontCamera.SetOrthoBoundary(-phi / 2, phi / 2, -phi / 2, phi / 2, 0, width);
        backCamera.SetOrthoBoundary(-phi / 2, phi / 2, -phi / 2, phi / 2, 0, width);

        // log
        std::cout << "[AABB box] maxX:" << maxX << " minX:" << minX
            << " maxY:" << maxY << " minY:" << minY
            << " maxZ:" << maxZ << " minZ:" << minZ << std::endl;
        std::cout << "[AABB box] width:" << width << " height:" << height << " length:" << length
            << " phi:" << phi << std::endl;
        std::cout << "[AABB box] centroid: " << "(" << centroid.x << ", " << centroid.y << ", " << centroid.z << ")\n";
        std::cout << "[AABB box] origin: " << "(" << origin.x << ", " << origin.y << ", " << origin.z << ")\n";
        std::cout << "[front camera] front vector:" << frontCamera.Front.z << std::endl;
        std::cout << "[back camera]  front vector:" << backCamera.Front.z << std::endl;
    }

    /*
     * change local position of the box to world space 
     */
    void toWorldPosition(const glm::mat4& modelMatrix)
    {
        glm::vec3 maxPosition = modelMatrix * glm::vec4(maxX, maxY, maxZ, 1.0f);
        glm::vec3 minPosition = modelMatrix * glm::vec4(minX, minY, minZ, 1.0f);
        maxX = maxPosition.x;
        maxY = maxPosition.y;
        maxZ = maxPosition.z;
        minX = minPosition.x;
        minY = minPosition.y;
        minZ = minPosition.z;

        std::cout << "Box World Space\n";
        setBox();
    }

    /*
     * whether a point is in the collision box
     */
    bool collideWithPoint(const glm::vec3& point)
    {
        glm::vec3 delta = glm::abs(point - centroid);
        return (delta.x < length / 2 && delta.y < height / 2 && delta.z < width / 2);
    }

    /*
     * position transform, under the perspective of front camera
     */
    glm::vec3 getFrontPosition(const glm::vec3& point)
    {
        // world space -> box space
        glm::vec3 boxPosition = point - origin;
        // box space -> front camera space
        float x = (boxPosition.x + phi / 2) * scr_width / phi;
        float y = boxPosition.y * scr_height / phi;
        float z = (width - boxPosition.z) / width;
        return glm::vec3(x, y, z);
    }

    /*
     * position transform, under the perspective of back camera  
     */
    glm::vec3 getBackPosition(const glm::vec3& point)
    {
        // world space -> box space
        glm::vec3 boxPosition = point - origin;
        // box space -> back camera space
        float x = (phi / 2 - boxPosition.x) * scr_width / phi;
        float y = boxPosition.y * scr_height / phi;
        float z = boxPosition.z / width;
        return glm::vec3(x, y, z);
    }
};

#endif