#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
layout(location = 5) in int BoneIDs[4];
layout(location = 6) in float Weights[4];

out vec2 TexCoords;
out vec3 position;
out vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aTexCoords;    
    position = aPos;
    normal = aNormal;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}