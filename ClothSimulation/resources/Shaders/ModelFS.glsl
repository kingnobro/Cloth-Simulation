#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 position;
in vec3 normal;

uniform sampler2D texture_diffuse1;

void main()
{   
    vec3 lightPos = vec3(0.0f, 5.0f, 0.0f);
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

    // Diffuse
    vec3 lightDir = normalize(lightPos - position);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 outColor = diffuse * vec3(0.83, 0.95, 0.90) + vec3(0.6, 0.6, 0.6);

    // FragColor = texture(texture_diffuse1, TexCoords);
    FragColor = vec4(outColor, 1.0f);
}