#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D diffuseMap;
uniform bool      hasDiffuse;
uniform vec3      matKd;

#define MAX_STARS 4
uniform vec3 starDirs[MAX_STARS];   // unit vectors pointing FROM fragment TOWARD each star
uniform vec3 starColors[MAX_STARS];
uniform int  numStars;

uniform vec3 ambientColor;
uniform vec3 viewPos;

void main()
{
    vec3 baseColor = hasDiffuse ? texture(diffuseMap, TexCoord).rgb : matKd;
    vec3 norm      = normalize(Normal);
    vec3 viewDir   = normalize(viewPos - FragPos);

    vec3 result = ambientColor * baseColor;

    for (int i = 0; i < numStars; i++) {
        // Diffuse
        float diff    = max(dot(norm, starDirs[i]), 0.0);
        vec3  diffuse = starColors[i] * diff * baseColor;

        // Specular (Blinn-Phong)
        vec3  halfway  = normalize(starDirs[i] + viewDir);
        float spec     = pow(max(dot(norm, halfway), 0.0), 32.0);
        vec3  specular = starColors[i] * spec * 0.4;

        result += diffuse + specular;
    }

    FragColor = vec4(result, 1.0);
}
