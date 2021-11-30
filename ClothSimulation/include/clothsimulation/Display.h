#pragma once

#include <iostream>

#include "Cloth.h"
#include "Rigid.h"
#include "stb_image.h"
#include "Camera.h"
#include "Shader.h"
#include "Model.h"

glm::vec3 lightPos(-5.0f, 7.0f, 6.0f);
glm::vec3 lightColor(0.7f, 0.7f, 1.0f);
Camera camera(glm::vec3(0.0f, 7.0f, 15.0f));