#pragma once

#include "assets.hpp"

#include <vector>
#include <chrono>

#include <GLFW/glfw3.h>


class App {
public:
    App();
    ~App();

    bool init(void);
    void init_assets(void);
    void init_callbacks(void);
    int run(void);

    void print_gl_info(void);

    void error_callback(int error, const char* description);
    void scroll_callback(double xoffset, double yoffset);
    void key_callback(int key, int scancode, int action, int mods);

private:
    GLFWwindow* window;

    GLuint shader_prog_ID{ 0 };
    GLuint VBO_ID{ 0 };
    GLuint VAO_ID{ 0 };

    bool vsync = true;

    std::vector<vertex> triangle_vertices =
    {
        {{0.0f,  0.5f,  0.0f}},
        {{0.5f, -0.5f,  0.0f}},
        {{-0.5f, -0.5f,  0.0f}}
    };

};

