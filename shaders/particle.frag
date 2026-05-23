#version 330 core
in  vec4 vColor;
out vec4 FragColor;

void main()
{
    vec2  c = gl_PointCoord - 0.5;
    float d = length(c) * 2.0;
    if (d > 1.0) discard;
    float a = (1.0 - smoothstep(0.0, 1.0, d)) * vColor.a;
    FragColor = vec4(vColor.rgb, a);
}
