#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
layout(location = 5) in int BoneIDs[4];
layout(location = 6) in float Weights[4];

out vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    normal = aNormal;
    float tolerance = 2.0f;  // 将顶点往法线方向扩展, 提早检测到碰撞, 避免点撞入模型
    gl_Position = projection * view * model * vec4(aPos + tolerance * aNormal, 1.0f);
}