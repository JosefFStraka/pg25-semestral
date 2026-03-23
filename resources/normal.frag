#version 460 core

layout(location = 0) in vec3 in_vertex_normal;

out vec4 FragColor;

void main()
{
    FragColor = vec4(in_vertex_normal, 1.f);
}
