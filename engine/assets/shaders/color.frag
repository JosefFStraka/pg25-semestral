#version 460 core

uniform vec4 uColor = vec4(1.0);

out vec4 FragColor;

void main() {
    FragColor = uColor;
}