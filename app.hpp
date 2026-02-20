#pragma once

#include "assets.hpp"

#include <vector>
#include <GLFW/glfw3.h>


class App {
public:
    App();
    ~App();

    bool init(void);
    void init_assets(void);
    int run(void);

private:

    GLFWwindow* window;

    GLuint shader_prog_ID{ 0 };
    GLuint VBO_ID{ 0 };
    GLuint VAO_ID{ 0 };

    std::vector<vertex> triangle_vertices =
    {
        {{0.0f,  0.5f,  0.0f}},
        {{0.5f, -0.5f,  0.0f}},
        {{-0.5f, -0.5f,  0.0f}}
    };

};

