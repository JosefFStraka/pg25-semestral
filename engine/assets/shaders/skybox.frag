#version 460 core
out vec4 FragColor;

in vec3 direction;

uniform samplerCube skybox;

void main()
{   
    vec3 hfDir = vec3(-direction.x, direction.y, direction.z);
    FragColor = texture(skybox, hfDir);
    //FragColor = vec4(direction, 1.0);
}
