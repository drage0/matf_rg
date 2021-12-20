#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv_;
layout (location = 2) in vec3 vnorm_;

out vec2 uv;
out vec3 vnorm;
out vec3 fpos;

uniform mat4 mvp;

void main()
{
    uv = uv_;
    vnorm = vnorm_;
    fpos = pos;

    gl_Position = mvp*vec4(fpos, 1.0);
}
