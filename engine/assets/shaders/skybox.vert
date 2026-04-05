#version 460 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture_coords;

out vec3 direction;

uniform mat4 uV_m;
uniform mat4 uP_m;

void main()
{
    vec4 pos = vec4((position * 2.0) - 1.0, 1.0f).xyww;
    direction = (inverse(uP_m * uV_m) * pos).xyz;
    gl_Position = pos;
}
