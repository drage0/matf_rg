#version 330 core

in vec2 uv;
in vec3 vnorm;
in vec3 fpos;

out vec4 colour;

uniform vec3 ambient_in = vec3(0.0f, 0.0f, 0.0f);
uniform vec4 specular_in = vec4(0.0f, 0.0f, 0.0f, 20.0f);
uniform vec3 eye;

void main()
{
    vec3 distant_light = eye;
    vec3 distant_light_dir = normalize(distant_light - fpos);

    float diffuse_factor = max(dot(normalize(vnorm), distant_light_dir), 0.0);
    vec3 diffuse = diffuse_factor*vec3(0.6, 0.6, 0.6);

    vec3 viewing_dir = normalize(eye - fpos);
    vec3 reflection = reflect(-distant_light_dir, vnorm);

    float specular_factor = pow(max(dot(viewing_dir, reflection), 0.0), specular_in.a);
    vec3 specular = specular_factor*specular_in.rgb;

    colour = vec4(ambient_in+diffuse+specular, 1.0f);
    
    /*
    colour.r = abs(vnorm.x);
    colour.g = abs(vnorm.y);
    colour.b = abs(vnorm.z);
    */
}
