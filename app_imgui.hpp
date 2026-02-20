#pragma once

struct GLFWwindow;
class App;

class AppImGui {
public:
    AppImGui(App* a, GLFWwindow* w);
    ~AppImGui();

    void init();
    void new_frame();
    void poll();
    void gui(double delta_time);
    void render();

    bool capture_mouse();
    bool capture_keyboard();

    bool imgui_open = false;
    bool debug_window_open = false;
private:
    GLFWwindow* window = nullptr;
    App* app = nullptr;
};
