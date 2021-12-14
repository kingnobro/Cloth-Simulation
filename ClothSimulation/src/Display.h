#pragma once

#include <iostream>

#include "Cloth.h"
#include "stb_image.h"
#include "Camera.h"
#include "Shader.h"
#include "Model.h"
#include "MouseRay.h"
#include "ClothPicker.h"
#include "ClothSewMachine.h"

// Light
glm::vec3 lightPos(-5.0f, 7.0f, 6.0f);
glm::vec3 lightColor(0.7f, 0.7f, 1.0f);

// Camera
Camera camera(glm::vec3(0.0f, 7.0f, 15.0f));

// Cloths
Cloth cloth1("assets/cloth/woman-shirt.dxf", glm::vec3(-2, 9, -1));
// Cloth cloth2("assets/cloth/shirt", glm::vec3(-2, 9, -4));
std::vector<Cloth*> cloths = { 
    &cloth1, 
    // &cloth2,
};
Cloth* selectedCloth = nullptr; // 需要移动的衣片

// 3D 点击选中物体功能
MouseRay mouseRay = MouseRay(&camera);
ClothPicker clothPicker = ClothPicker(&camera);

// 缝纫机
ClothSewMachine sewMachine = ClothSewMachine(&camera);