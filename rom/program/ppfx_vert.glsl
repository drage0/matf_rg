#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 vuv;

out vec2 uv;

void main()
{
    uv = vuv;
    uv.x *= -1;
    gl_Position = vec4(vec3(pos.x, pos.y, pos.z), 1.0);
}  
