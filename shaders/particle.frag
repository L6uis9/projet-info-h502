#version 330 core
in  vec4 vColor;
in  vec2 vUV;
out vec4 FragColor;

void main()
{
    float d = length(vUV);
    if (d > 1.0) discard;
    float a = (1.0 - smoothstep(0.0, 1.0, d)) * vColor.a;
    FragColor = vec4(vColor.rgb, a);
}
