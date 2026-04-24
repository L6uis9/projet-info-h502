#version 330 core

in  vec2 vUV;
out vec4 FragColor;

uniform vec3 starColor;

void main()
{
    vec2  c = vUV - 0.5;
    float d = length(c) * 2.0;
    if (d > 1.0) discard;

    float core = 1.0 - smoothstep(0.0, 0.5, d);
    float glow = 1.0 - smoothstep(0.1, 1.0, d);
    glow = pow(glow, 1.5);

    // Core blends toward white, glow keeps the star colour
    vec3 coreColor = mix(starColor, vec3(1.0), 0.6);
    vec3 glowColor = starColor;

    vec3  color = mix(glowColor, coreColor, core);
    float alpha = core + glow * 0.85;
    alpha = min(alpha, 1.0);

    FragColor = vec4(color, alpha);
}
