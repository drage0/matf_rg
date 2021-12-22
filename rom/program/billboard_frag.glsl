#version 330 core

in vec2 uv;

out vec4 colour;

uniform sampler2D imgtexture;

void main()
{
    colour = texture(imgtexture, uv);
    if (colour.r == 0.0f && colour.g >= 1.0f && colour.b >= 1.0f)
    {
        discard;
    }
}
