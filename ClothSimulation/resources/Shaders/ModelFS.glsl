#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 position;
in vec3 normal;

uniform sampler2D texture_diffuse1;

void main()
{   
    vec3 outColor = normal * 0.5f + 0.5f;
    // FragColor = texture(texture_diffuse1, TexCoords);
    FragColor = vec4(outColor, 1.0f);
}