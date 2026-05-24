#version 330 core
layout(location = 0) in vec3  vertex;   // quad corner offset: (-1,-1,0) .. (1,1,0)
layout(location = 1) in vec4  center;   // xyz = world position, w = size
layout(location = 2) in vec4  col;

out vec4 vColor;
out vec2 vUV;

uniform mat4 V;
uniform mat4 P;
uniform vec3 cameraRight;
uniform vec3 cameraUp;

void main()
{
    float scale = center.w;
    vec3 worldPos = center.xyz
                  + cameraRight * vertex.x * scale
                  + cameraUp    * vertex.y * scale;
    gl_Position = P * V * vec4(worldPos, 1.0);
    vColor = col;
    vUV    = vertex.xy;
}
