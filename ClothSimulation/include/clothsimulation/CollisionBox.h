#pragma once

#include <float.h>
#include <glm/glm.hpp>

class CollisionBox
{
public:
	float width;
	float height;
	float length;
	float phi;		// max(height, length)
	glm::vec3 centroid;

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

	// set width, height length of the AABB box
	void setBox()
	{
		width = maxZ - minZ;
		height = maxY - minY;
		length = maxX - minX;
		phi = max(length, height);
		centroid = glm::vec3((maxX + minX) / 2, (maxY + minY) / 2, (maxZ + minZ) / 2);

		std::cout << "[AABB box] maxX:" << maxX << " minX:" << minX
			<< " maxY:" << maxY << " minY:" << minY
			<< " maxZ:" << maxZ << " minZ:" << minZ << std::endl;
		std::cout << "[AABB box] width:" << width << " height:" << height << " length:" << length
			<< " phi:" << phi << std::endl;
		std::cout << "[AABB box] centroid: " << "(" << centroid.x << ", " << centroid.y << ", " << centroid.z << ")\n";
	}

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