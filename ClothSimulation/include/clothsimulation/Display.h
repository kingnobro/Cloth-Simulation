#pragma once

#include <iostream>

#include "Cloth.h"
#include "Rigid.h"
#include "stb_image.h"
#include "Camera.h"
#include "Shader.h"
#include "Model.h"
#include "MouseRay.h"
#include "ClothPicker.h"
#include "ClothSewMachine.h"

// Gravity
extern const int iterationFreq = 10;
extern glm::vec3 gravity(0.0, -9.8 / iterationFreq, 0.0);

// Light
glm::vec3 lightPos(-5.0f, 7.0f, 6.0f);
glm::vec3 lightColor(0.7f, 0.7f, 1.0f);

// Camera
Camera camera(glm::vec3(0.0f, 7.0f, 15.0f));

// Cloths
int clothNumber = 0;
glm::vec2 bigClothSize(6, 6);
glm::vec2 smallClothSize(1, 1);
//			 Position                  Size       clothID
Cloth cloth1(glm::vec3(-3, 9, -1), bigClothSize, ++clothNumber);
Cloth cloth2(glm::vec3(-3, 9, -4), bigClothSize, ++clothNumber);
std::vector<Cloth*> cloths = { &cloth1, &cloth2 };
Cloth* selectedCloth = nullptr; // 需要移动的衣片

// 3D raycast picker
MouseRay mouseRay = MouseRay(&camera);
ClothPicker clothPicker = ClothPicker(&camera);

// Sew Machine
ClothSewMachine sewMachine = ClothSewMachine(&camera);