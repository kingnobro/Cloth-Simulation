#ifndef _COMMON_HPP
#define _COMMON_HPP

struct ClothCollision {
	float sphereR;
	float cellUnit;
	glm::vec3 spaceMin;
	glm::vec3 spaceMax;
	glm::vec3 spaceDim;
	int* cellArray;

	ClothCollision(float sphereR, float cellUnit, glm::vec3 spaceMin, glm::vec3 spaceMax) {
		this->sphereR = sphereR;
		this->cellUnit = cellUnit;
		this->spaceMin = spaceMin;
		this->spaceMax = spaceMax;
		this->spaceDim = glm::vec3(
			(spaceMax.x - spaceMin.x) / cellUnit,
			(spaceMax.y - spaceMin.y) / cellUnit,
			(spaceMax.z - spaceMin.z) / cellUnit);
		cellArray = new int[int(spaceDim.x) * int(spaceDim.y) * int(spaceDim.z) + 1];
	}

	~ClothCollision() {
		delete[]cellArray;
	}

	unsigned int hashCellID(glm::vec3 in_pos) {
		// if position out of cell hash space
		if (in_pos.x < spaceMin.x || in_pos.y < spaceMin.y || in_pos.z < spaceMin.z ||
			in_pos.x > spaceMin.x + spaceDim.x * cellUnit ||
			in_pos.y > spaceMin.y + spaceDim.y * cellUnit ||
			in_pos.z > spaceMin.z + spaceDim.z * cellUnit) {
			return 0;
		}
		else {
			int cell_x = floor((in_pos.x - spaceMin.x) / cellUnit);
			int cell_y = floor((in_pos.y - spaceMin.y) / cellUnit);
			int cell_z = floor((in_pos.z - spaceMin.z) / cellUnit);

			return cell_y * spaceDim.x * spaceDim.z + cell_z * spaceDim.x + cell_x;
		}
	}

	// find cell position of a node
	// @param: in_pos is the worldposition of a node
	glm::vec3 getCellCoord(glm::vec3 in_pos) {
		return glm::vec3(
			spaceMin.x + floor((in_pos.x - spaceMin.x) * cellUnit) * cellUnit,
			spaceMin.y + floor((in_pos.y - spaceMin.y) * cellUnit) * cellUnit,
			spaceMin.z + floor((in_pos.z - spaceMin.z) * cellUnit) * cellUnit
		);
	}
};

#endif
