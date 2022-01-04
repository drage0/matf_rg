#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv_;
layout (location = 2) in vec3 vnorm_;
layout (location = 3) in vec3 vtangent_;
layout (location = 4) in vec3 vbitangent_;

out vec2 uv;
out vec3 fpos;
out vec3 TangentLightPos;
out vec3 TangentDistantLightPos;
out vec3 TangentFragPos;
out vec3 TangentViewPos;

uniform mat4 mvp;
uniform vec3 eye;
uniform mat4 model;
uniform vec3 distant_light_dir_in = vec3(32.0f, 8.0f, 1.0f);

void main()
{
    uv = uv_;
    fpos = vec3(model * vec4(pos, 1.0));

    vec3 T = normalize(mat3(model) * vtangent_);
    vec3 B = normalize(mat3(model) * vbitangent_);
    vec3 N = normalize(mat3(model) * vnorm_);
    mat3 TBN = transpose(mat3(T, B, N));
    TangentLightPos = TBN * vec3(eye.x, eye.y, eye.z);
    TangentViewPos  = TBN * vec3(eye.x, eye.y, eye.z);
    TangentFragPos  = TBN * fpos;
    TangentDistantLightPos = TBN*distant_light_dir_in;

    gl_Position = mvp*vec4(fpos, 1.0);
}
