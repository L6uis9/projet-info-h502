#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D diffuseMap;
uniform bool      hasDiffuse;

uniform vec3  lightDir;      // world-space, normalised
uniform vec3  lightColor;
uniform vec3  ambientColor;
uniform vec3  viewPos;

// Flat Kd fallback when no texture
uniform vec3  matKd;

void main()
{
    // Base colour
    vec3 baseColor = hasDiffuse ? texture(diffuseMap, TexCoord).rgb : matKd;

    // Ambient
    vec3 ambient = ambientColor * baseColor;

    // Diffuse
    vec3 norm    = normalize(Normal);
    float diff   = max(dot(norm, -lightDir), 0.0);
    vec3 diffuse = lightColor * diff * baseColor;

    // Specular (Blinn-Phong)
    vec3 viewDir  = normalize(viewPos - FragPos);
    vec3 halfway  = normalize(-lightDir + viewDir);
    float spec    = pow(max(dot(norm, halfway), 0.0), 32.0);
    vec3 specular = lightColor * spec * 0.4;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
