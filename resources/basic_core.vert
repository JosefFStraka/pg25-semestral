#version 460 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture_coords;

uniform mat4 uP_m = mat4(1.0);
uniform mat4 uM_m = mat4(1.0);
uniform mat4 uV_m = mat4(1.0);

void main()
{
    // Outputs the positions/coordinates of all vertices
    gl_Position = uP_m * uV_m * uM_m * vec4(position, 1.0f);
}