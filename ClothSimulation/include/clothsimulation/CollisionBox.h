#pragma once

#include <float.h>
#include <glm/glm.hpp>

#include "Camera.h"

/*
 * 包围 3D 模型的碰撞盒, 盒体前后有两个摄像机, 用于生成深度图和法线图, 深度图用于碰撞检测
 */
class CollisionBox
{
public:
	float width;
	float height;
	float length;
	float phi;			// projection area of the orthographic camera, phi = max(height, length)
	int mapsize;		
	glm::vec3 centroid; // 包围盒的中心
	glm::vec3 origin;	// 包围盒空间的坐标原点, 在后部摄像机的正下方
	Camera frontCamera;	// Camera around AABB box for depth map generation
	Camera backCamera;

	CollisionBox(int mapsize = 512)
	{
		minX = minY = minZ = FLT_MAX;
		maxX = maxY = maxZ = FLT_MIN;
		width = height = length = phi = 0;
		centroid = glm::vec3(0.0f);
		// todo: fix hard code
		this->mapsize = mapsize;
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
		phi = max(length, height);
		centroid = glm::vec3((maxX + minX) / 2, (maxY + minY) / 2, (maxZ + minZ) / 2);
		origin = centroid - glm::vec3(0.0f, height / 2, width / 2);

		// update front and back camera
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 frontPosition = centroid + glm::vec3(0.0f, 0.0f, width / 2);
		glm::vec3 backPosition = centroid - glm::vec3(0.0f, 0.0f, width / 2);
		float frontYaw = -90.0f;
		float backYaw = 90.0f;
		frontCamera = Camera(frontPosition, up, frontYaw);
		backCamera = Camera(backPosition, up, backYaw);
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
		float x = (boxPosition.x + phi / 2) * mapsize / phi;
		float y = boxPosition.y * mapsize / phi;
		float z = (width - boxPosition.z) / width;
		return glm::vec3(x, y, z);
	}

	glm::vec3 inverseFrontPosition(const glm::vec3& point)
	{
		float x = point.x * phi / mapsize - phi / 2;
		float y = point.y * phi / mapsize;
		float z = width - point.z * width;
		return glm::vec3(x, y, z) + origin;
	}

	/*
	 * position transform, under the perspective of back camera  
	 */
	glm::vec3 getBackPosition(const glm::vec3& point)
	{
		// world space -> box space
		glm::vec3 boxPosition = point - origin;
		// box space -> back camera space
		float x = (phi / 2 - boxPosition.x) * mapsize / phi;
		float y = boxPosition.y * mapsize / phi;
		float z = boxPosition.z / width;
		return glm::vec3(x, y, z);
	}

private:
	float minX, minY, minZ;
	float maxX, maxY, maxZ;
};