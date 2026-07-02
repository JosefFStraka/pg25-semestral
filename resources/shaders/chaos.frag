#version 460 core
out vec4 FragColor;

uniform float uTime;

#define MAX_LIGHTS 16
struct s_lights {
    vec4 position[MAX_LIGHTS];
    vec4 direction[MAX_LIGHTS];
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

uniform float uAlpha = 1.0;
uniform float uChaosOffset = 1.0;
uniform float uChaosExp = 5.0;

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

    return vec4(ambient + diffuse + specular, 1.0);
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

    return vec4((ambient + diffuse + specular) * att, 1.0);
}

vec4 SpotLight(int i, vec3 N, vec3 V) { 
    return vec4(0.0);
}

float get_grayscale(vec3 color) {
    return  0.299 * color.r + 0.587 * color.g + 0.114 * color.b;
}

float get_random2d(vec2 st, float add) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233)))* 43758.123 + add * 52342.124);
}

void main()
{
    vec3 N = normalize(fs_in.N);
    vec3 V = normalize(-fs_in.V);

    if(!gl_FrontFacing) {  // transparent, backface culling is OFF => backface is rasterized 
        N = -N;
    }

    vec4 accumulator = vec4(0.0);    
    for (int i = 0; i < active_lights; i++) {        
        if (lights.position[i].w == 0.0)
            accumulator += DirectionalLight(i, N, V);       
        else if (lights.spotCutoff[i] == 180.0)
            accumulator += PointLight(i, N, V);        
        else            
            accumulator += SpotLight(i, N, V);
    }  

    float grayscale = clamp(get_grayscale(accumulator.rgb), 0.0, 1.0);
    float rnd = get_random2d(fs_in.T, mod(uTime, 1.0));
    float chaos = clamp(pow(rnd, pow(fract(grayscale + uChaosOffset), uChaosExp)), 0.0, 1.0);
    FragColor =  vec4(chaos, chaos, chaos, uAlpha);
}
