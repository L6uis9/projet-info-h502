#version 330 core
layout(location = 0) in vec3  aPos;
layout(location = 1) in vec4  aColor;
layout(location = 2) in float aSize;

out vec4 vColor;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 viewPos = view * vec4(aPos, 1.0);
    gl_Position  = projection * viewPos;
    float dist   = max(length(viewPos.xyz), 0.5);
    gl_PointSize = aSize * 120.0 / dist;
    vColor       = aColor;
}
