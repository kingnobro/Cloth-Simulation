#ifndef DISPLAY_H
#define DISPLAY_H

#include "ClothCreator.h"
#include "MouseRay.h"
#include "ClothPicker.h"
#include "ClothSewMachine.h"

// Light
glm::vec3 lightPos(-5.0f, 7.0f, 6.0f);
glm::vec3 lightColor(0.7f, 0.7f, 1.0f);

// Camera
Camera camera(glm::vec3(-1.0f, 20.0f, 16.0f));

// Cloths
ClothCreator clothCreator = ClothCreator("assets/cloth/woman-shirt.dxf");
std::vector<Cloth*>& cloths = clothCreator.cloths;
Cloth* selectedCloth = nullptr; // 鼠标选中的衣片

// 3D 点击选中物体功能
MouseRay mouseRay = MouseRay(&camera);
ClothPicker clothPicker = ClothPicker(&camera);

// 缝纫机
ClothSewMachine sewMachine = ClothSewMachine(&camera);

#endif