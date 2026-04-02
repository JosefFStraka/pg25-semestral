#version 460 core
out vec4 FragColor;

uniform vec3 ambient_material = vec3(1.0, 1.0, 1.0);
uniform vec3 ambient_intensity = vec3(0.1, 0.1, 0.1);

uniform sampler2D tex_diffuse;
uniform vec3 diffuse_intensity = vec3(1.0, 1.0, 1.0);

uniform vec3 specular_material = vec3(1.0, 1.0, 1.0);
uniform vec3 specular_intensity = vec3(1.0, 1.0, 1.0);
uniform float specular_shinines = 12.0;

in VS_OUT {
    vec3 N;
    vec2 T;
    vec3 L;
    vec3 V;
} fs_in;

void main()
{
    // Normalize the incoming N, L and V vectors
    vec3 N = normalize(fs_in.N);
    vec3 L = normalize(fs_in.L);
    vec3 V = normalize(fs_in.V);

    // Calculate R by reflecting -L around the plane defined by N
    vec3 R = reflect(-L, N);

    vec3 ambient = ambient_material * ambient_intensity;

    vec3 diffuse = diffuse_intensity * max(0.0, dot(N, L));
    vec3 specular = specular_material * specular_intensity * pow(max(0.0, dot(R, V)), specular_shinines);

    FragColor = vec4( (ambient + diffuse) * texture(tex_diffuse, fs_in.T).rgb + specular, 1.0);
}
