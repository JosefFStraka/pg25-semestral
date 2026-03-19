#version 460 core
in vec3 position;
in vec3 notmal;
in vec2 texture_coords;

uniform mat4 uP_m = mat4(1.0f);
uniform mat4 uM_m = mat4(1.0f);
uniform mat4 uV_m = mat4(1.0f);

void main()
{
    // Outputs the positions/coordinates of all vertices
    gl_Position = vec4(position, 1.0f); //uP_m * uV_m * uM_m * vec4(position, 1.0f);
}
