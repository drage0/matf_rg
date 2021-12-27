#version 330 core

in vec2 uv;

out vec4 colour;

uniform sampler2D imgtexture;
uniform vec2 display_resolution;

void main()
{
    vec2 position = (gl_FragCoord.xy / display_resolution) - vec2(0.5);           
    float dist = length(position * vec2(display_resolution.x/display_resolution.y, 1.0));

    float radius = 0.99;
    float softness = 0.667;
    float vignette = smoothstep(radius, radius - softness, dist);

    colour.rgb = texture(imgtexture, uv).rgb * ( vignette);
    colour.a = 1.0f;
}
