#version 460 core
out vec4 FragColor;

#define MAX_LIGHTS 16
struct s_lights {
    vec4 position[MAX_LIGHTS];
    vec4 color[MAX_LIGHTS];
    float attenuation[MAX_LIGHTS];
    float spotCutoff[MAX_LIGHTS];
};
uniform s_lights lights;
uniform int active_lights; // active light count

uniform vec3 ambient_material = vec3(1.0, 1.0, 1.0);
uniform vec3 ambient_intensity = vec3(0.1, 0.1, 0.1);

uniform vec3 diffuse_intensity = vec3(1.0, 1.0, 1.0);

uniform vec3 specular_material = vec3(1.0, 1.0, 1.0);
uniform vec3 specular_intensity = vec3(1.0, 1.0, 1.0);
uniform float specular_shinines = 18.0;

uniform vec3 radiation = vec3(0.0);

uniform int debugMode = 0;

uniform sampler2D tex_diffuse;

uniform mat4 uV_m = mat4(1.0f);

in VS_OUT {
    vec3 N;
    vec2 T;
    vec3 V;
} fs_in;

vec4 DirectionalLight(int i, vec3 N, vec3 V) {
    vec3 lightColor = lights.color[i].rgb;

    vec3 L = normalize(uV_m * -lights.position[i]).xyz;

    // Calculate R by reflecting -L around the plane defined by N
    vec3 R = reflect(-L, N);

    vec3 ambient = ambient_material * ambient_intensity;
    vec3 diffuse = lightColor * diffuse_intensity * max(0.0, dot(N, L));
    vec3 specular = lightColor * specular_material * specular_intensity * pow(max(0.0, dot(R, V)), specular_shinines);

    vec4 finalColor = vec4(0.7);
    if (debugMode == 3)
        finalColor = vec4(ambient, 1.0);
    else if (debugMode == 4)
        finalColor = vec4(diffuse, 1.0);
    else if (debugMode == 5)
        finalColor = vec4(specular, 1.0);
    else
        finalColor = vec4(ambient + diffuse + specular, 1.0);

    return finalColor;
}

vec4 PointLight(int i, vec3 N, vec3 V) {
    vec3 lightPos = (uV_m * vec4(lights.position[i].xyz, 1.0)).xyz;
    vec3 lightColor = lights.color[i].rgb;

    vec3 Lnn = lightPos - fs_in.V;
    vec3 L = normalize(Lnn);
    // Calculate R by reflecting -L around the plane defined by N
    vec3 R = reflect(-L, N);

    vec3 ambient = ambient_material * ambient_intensity;
    vec3 diffuse = lightColor * diffuse_intensity * max(0.0, dot(N, L));
    vec3 specular = lightColor * specular_material * specular_intensity * pow(max(0.0, dot(R, V)), specular_shinines);
    
    float dist = length(Lnn);
    float att = 1.0 / (1.0 + lights.attenuation[i] * dist * dist);

    vec4 finalColor = vec4(0.7);
    if (debugMode == 3)
        finalColor = vec4(ambient, 1.0);
    else if (debugMode == 4)
        finalColor = vec4(diffuse, 1.0);
    else if (debugMode == 5)
        finalColor = vec4(specular, 1.0);
    else
        finalColor = vec4((ambient + diffuse + specular) * att, 1.0);

    return finalColor;
}

vec4 SpotLight(int i, vec3 N, vec3 V) { 
    return vec4(0.0);
}

void main()
{
    vec3 albedo = texture(tex_diffuse, fs_in.T).rgb;

    vec3 N = normalize(fs_in.N);
    vec3 V = normalize(-fs_in.V);

    vec4 accumulator = vec4(0.0);    
    for (int i = 0; i < active_lights; i++) {        
        if (lights.position[i].w == 0.0)
            accumulator += DirectionalLight(i, N, V);       
        else if (lights.spotCutoff[i] == 180.0)
            accumulator += PointLight(i, N, V);        
        else            
            accumulator += SpotLight(i, N, V);
    }  

    vec4 finalColor = vec4(0.7);
    if (debugMode == 0)
        finalColor = vec4(radiation + accumulator.rgb * albedo, 1.0);
    else if (debugMode == 1)
        finalColor = vec4(N * 0.5 + 0.5, 1.0);
    else if (debugMode == 2)
        finalColor = vec4(V * 0.5 + 0.5, 1.0);
    else
        finalColor = vec4(accumulator.rgb, 1.0);
        
    FragColor = finalColor;
}
