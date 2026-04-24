#version 330 core

layout(location = 0) in vec2 aPos;  // -0.5..0.5 quad

out vec2 vUV;

uniform mat4  view;
uniform mat4  projection;
uniform vec3  sunDir;      // unit vector toward sun
uniform float sunRadius;   // half-size in NDC y units

void main()
{
    vec4 center = projection * mat4(mat3(view)) * vec4(sunDir, 1.0);

    float aspect = projection[1][1] / projection[0][0];

    vec2 offset = vec2(aPos.x / aspect, aPos.y) * sunRadius * 2.0 * center.w;

    gl_Position = vec4(center.xy + offset, center.w, center.w); // depth = 1.0 after divide

    vUV = aPos + 0.5;
}
