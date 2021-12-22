#version 330 core

layout (location = 0) in vec3 pos;

out vec3 uv;

uniform mat4 mvp;

void main()
{
    uv = vec3(pos.x, -pos.y, pos.z);
    gl_Position = mvp * vec4(vec3(pos.x*1, pos.y*1, pos.z*1), 1.0);
}  
