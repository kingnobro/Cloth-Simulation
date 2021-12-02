#version 330 core
out vec4 FragColor;

in vec3 normal;

void main()
{
    // replace color with normal vector
    // color:[0,1] normal:[-1,1], coordinate needs transforming
    vec3 color = normal * 0.5 + vec3(0.5f);
    FragColor = vec4(color, 1.0f);
}