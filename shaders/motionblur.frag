#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform float blurStrength;

const int NUM_SAMPLES = 12;

void main()
{
    if (blurStrength < 0.0001) {
        FragColor = texture(screenTexture, TexCoord);
        return;
    }

    vec2 dir = TexCoord - vec2(0.5);
    vec4 color = vec4(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        float t      = float(i) / float(NUM_SAMPLES - 1);
        float weight = 1.0 - abs(t * 2.0 - 1.0);
        vec2 uv      = clamp(TexCoord + dir * (t - 0.5) * blurStrength, 0.0, 1.0);
        color       += texture(screenTexture, uv) * weight;
        totalWeight += weight;
    }

    FragColor = color / totalWeight;
}
