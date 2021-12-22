#version 330 core

in vec3 uv;

out vec4 colour;

uniform samplerCube skybox;

uniform float gamma = 1.7f;

void main()
{
    colour = texture(skybox, uv);
    colour.rgb = pow(colour.rgb, vec3(1.0/gamma));
}
