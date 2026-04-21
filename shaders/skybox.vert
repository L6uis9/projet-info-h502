#version 330 core

layout(location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;   // rotation only (no translation)

void main()
{
    TexCoords   = aPos;
    vec4 pos    = projection * view * vec4(aPos, 1.0);
    // Trick: set z = w so the skybox sits exactly at the far plane (depth = 1)
    gl_Position = pos.xyww;
}
