#pragma once

#include <vector>
#include <chrono>

#include "assets.hpp"
#include "settings.hpp"
#include "colors.hpp"

#include <GL/glew.h> 
#include <GL/wglew.h> 
#include <GLFW/glfw3.h>

class AppImGui;

class App {
public:
    App();
    ~App();

    bool init(void);
    void init_assets(void);
    void init_callbacks(void);
    void init_imgui(void);
    int run(void);

    void print_gl_info(void);

    //callbacks
    void error_callback(int error, const char* description);
    void key_callback(int key, int scancode, int action, int mods);
    void fbsize_callback(int width, int height);
    void window_pos_callback(int xpos, int ypos);
    void mouse_button_callback(int button, int action, int mods);
    void cursor_position_callback(double xpos, double ypos);
    void scroll_callback(double xoffset, double yoffset);

    settings::app_settings::AppSettings app_settings;
private:
    GLFWwindow* window;

    AppImGui* imgui;

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
