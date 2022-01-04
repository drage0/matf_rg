#version 330 core

in vec2 uv;
in vec3 fpos;
in vec3 TangentLightPos;
in vec3 TangentFragPos;
in vec3 TangentViewPos;
in vec3 TangentDistantLightPos;

out vec4 colour;

uniform vec3 diffuse_in = vec3(0.6f, 0.6f, 0.6f);
uniform vec3 ambient_in = vec3(0.0f, 0.0f, 0.0f);
uniform vec4 specular_in = vec4(0.0f, 0.0f, 0.0f, 20.0f);
uniform float transparency_in = 0.0f;
uniform sampler2D imgtexture;
uniform sampler2D normalmap;
uniform sampler2D parallaxmap;

void main()
{
    vec3 viewDir = normalize(TangentViewPos - TangentFragPos);

    //
    // Parallax
    //
    float pheight =  texture(parallaxmap, uv).r;    
    vec2 p = viewDir.xy / viewDir.z * (pheight * 0.6f);
    vec2 new_uv = uv - p;

    if(new_uv.x > 1.0 || new_uv.y > 1.0 || new_uv.x < 0.0 || new_uv.y < 0.0)
    {
        //discard;
    }

    vec3 normal = texture(normalmap, new_uv).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    
    //
    //
    // POINT LIGHT
    //
    //

    float distance    = length(TangentLightPos - TangentFragPos);
    float attenuation = 1.0 / (0.01f + 0.01f * distance + 0.003f * (distance * distance));   
   
    // get diffuse color
    vec3 color = diffuse_in*texture(imgtexture, new_uv).rgb*vec3(0.49f);
    // ambient
    vec3 ambient = ambient_in;
    // diffuse
    vec3 lightDir = normalize(TangentLightPos - TangentFragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;
    // specular
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), specular_in.a);

    vec3 specular = specular_in.rgb * vec3(2.0f) * spec;
    colour.rgb = vec3((ambient + diffuse + specular)*attenuation);
    
    //
    // DISTANT LIGHT /directional light/
    //
    //
    
    // get diffuse color
    color = diffuse_in*texture(imgtexture, new_uv).rgb*vec3(245.0f/255.0f, 235.0f/255.0f, 210.0f/255.0f)*1.288;
    // ambient
    ambient = ambient_in;
    // diffuse
    lightDir = normalize(TangentDistantLightPos);
    diff = max(dot(lightDir, normal), 0.0);
    diffuse = diff * color;
    colour.rgb += vec3(diffuse);



    // Gamma
    colour.rgb = pow(colour.rgb, vec3(1.0/0.688));
    colour.a = 1.0-transparency_in;

}
