#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 LocalPos;
in vec3 LocalNormal;

out vec4 FragColor;

uniform sampler2D   diffuseMap;
uniform samplerCube skyboxMap;
uniform bool        hasDiffuse;
uniform bool        useTriplanar;
uniform bool        reflective;
uniform vec3        matKd;

#define MAX_STARS 4
uniform vec3 starDirs[MAX_STARS];   // unit vectors pointing FROM fragment TOWARD each star
uniform vec3 starColors[MAX_STARS];
uniform int  numStars;

uniform vec3 ambientColor;
uniform vec3 viewPos;

void main()
{
    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    if (reflective) {
        vec3 I = -viewDir;
        vec3 R = reflect(I, norm);
        vec3 envColor = texture(skyboxMap, R).rgb;
        FragColor = vec4(envColor , 1.0);
        return;
    }

    vec3 baseColor;
    if (hasDiffuse && useTriplanar) {
        // Triplanar mapping: sample from 3 orthogonal planes and blend by normal
        // Uses object-space position/normal so texture sticks to the asteroid as it rotates
        vec3 w = abs(normalize(LocalNormal));
        w = pow(w, vec3(4.0));          // sharpen blend at axis-aligned faces
        w /= w.x + w.y + w.z;
        const float scale = 0.5;
        vec3 tx = texture(diffuseMap, LocalPos.yz * scale).rgb;
        vec3 ty = texture(diffuseMap, LocalPos.xz * scale).rgb;
        vec3 tz = texture(diffuseMap, LocalPos.xy * scale).rgb;
        baseColor = tx * w.x + ty * w.y + tz * w.z;
    } else {
        baseColor = hasDiffuse ? texture(diffuseMap, TexCoord).rgb : matKd;
    }

    vec3 result = ambientColor * baseColor;

    for (int i = 0; i < numStars; i++) {
        // Diffuse
        float NdotL    = dot(norm, starDirs[i]);
        float diff     = smoothstep(-0.05, 0.1, NdotL);
        vec3  lightCol = mix(vec3(1.0), starColors[i], 0.4);
        vec3  diffuse  = lightCol * diff * baseColor;

        // Specular (Blinn-Phong)
        vec3  halfway  = normalize(starDirs[i] + viewDir);
        float spec     = pow(max(dot(norm, halfway), 0.0), 32.0);
        vec3  specular = lightCol * spec * 0.4;

        result += diffuse + specular;
    }

    FragColor = vec4(result, 1.0);
}
