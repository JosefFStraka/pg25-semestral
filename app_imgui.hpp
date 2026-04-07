#pragma once

#include "Scene.hpp"
#include "ModelInstance.hpp"

struct GLFWwindow;

class AppImGui {
public:
    AppImGui(GLFWwindow* w);
    ~AppImGui();

    void init();
    void new_frame();
    void poll();
    void gui_begin();
    void gui_end();
    void render();

    bool capture_mouse();
    bool capture_keyboard();

    bool debug_window_open = false;

    static void model_controls(ModelInstance * const model);
    static void light_controls(s_lights * lights, int i);
private:
    GLFWwindow* window = nullptr;
};
