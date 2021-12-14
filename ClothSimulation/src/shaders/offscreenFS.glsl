#version 330 core
out vec4 FragColor;

in vec3 normal;

void main()
{
    // replace color with normal std::vector
    // color:[0,1] normal:[-1,1], coordinate needs transforming
    vec3 color = normal * 0.5f + vec3(0.5f);
    FragColor = vec4(color, 1.0f);
}