#pragma once

#include <float.h>
#include <glm/glm.hpp>

#include "Camera.h"

class CollisionBox
{
public:
	float width;
	float height;
	float length;
	float phi;			// max(height, length)
	glm::vec3 centroid;
	Camera frontCamera;	// Camera around AABB box for depth map generation
	Camera backCamera;

	CollisionBox()
	{
		minX = minY = minZ = FLT_MAX;
		maxX = maxY = maxZ = FLT_MIN;
		width = height = length = phi = 0;
		centroid = glm::vec3(0.0f);
	}

	// update the boundary of AABB box
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

	void setBox()
	{
		// set width, height length of the AABB box
		width = maxZ - minZ;
		height = maxY - minY;
		length = maxX - minX;
		phi = max(length, height);
		centroid = glm::vec3((maxX + minX) / 2, (maxY + minY) / 2, (maxZ + minZ) / 2);

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
		std::cout << "[front camera] front:" << frontCamera.Front.z << std::endl;
		std::cout << "[back camera]  front:" << backCamera.Front.z << std::endl;
	}

	// change local position to world position
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

private:
	float minX, minY, minZ;
	float maxX, maxY, maxZ;
};