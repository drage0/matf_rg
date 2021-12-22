#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 vuv;

out vec2 uv;

uniform mat4 mvp;
uniform vec2 screenwh;

void main()
{
    uv = vuv;
    gl_Position = mvp * vec4(vec3(pos.x, pos.y, pos.z), 1.0);
}  
