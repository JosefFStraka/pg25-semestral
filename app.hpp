#pragma once

#include <chrono>
#include <vector>

#include "assets.hpp"
#include "AxisDisplay.hpp"
#include "settings.hpp"
#include "colors.hpp"
#include "app_imgui.hpp"
#include "fps_meter.hpp"
#include "ShaderProgram.hpp"
#include "Mesh.hpp"
#include "Model.hpp"
#include "OBJloader.hpp"
#include "camera.hpp"

#include <GL/glew.h> 
#include <GL/wglew.h> 
#include <GLFW/glfw3.h>

class App {
public:
    App();
    ~App();

    bool init(void);
    int run(void);
    void gui(void);
private:
    void init_glfw(void);
    void init_glew(void);
    void init_gl_debug(void);
    void init_imgui(void);
    void init_assets(void);
    void init_callbacks(void);

    void print_gl_info(void);

    void update_projection_matrix(void);

    //callbacks
    void error_callback(int error, const char* description);
    void key_callback(int key, int scancode, int action, int mods);
    void fbsize_callback(int width, int height);
    void window_pos_callback(int xpos, int ypos);
    void mouse_button_callback(int button, int action, int mods);
    void cursor_position_callback(double xpos, double ypos);
    void scroll_callback(double xoffset, double yoffset);

    void set_fullscreen(bool value);
    void set_gui_enabled(bool value);
    void set_vsync(bool value);

    double last_cursor_pos_x{};
    double last_cursor_pos_y{};
    double saved_cursor_pos_x{};
    double saved_cursor_pos_y{};

    int saved_window_pos_x{};
    int saved_window_pos_y{};
    int saved_window_width{};
    int saved_window_height{};

    // projection related variables    
    int fb_width{0}, fb_height{0}; //fb size
    float fov = 60.0f;
    // store projection matrix here, update only on callbacks
    glm::mat4 projection_matrix = glm::identity<glm::mat4>();

    GLFWwindow* window;
    AppImGui* imgui;
    settings::app_settings::AppSettings app_settings;

    // camera related 
    Camera camera;
    // remember last cursor position, move relative to that in the next frame
    double cursorLastX{ 0 };
    double cursorLastY{ 0 };
    
    AxisDisplay axis_display;
    fps_meter FPS;

    // shared library of shaders for all models, automatic resource management 
    std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> shader_library;

    // shared library of meshes for all models, automatic resource management 
    std::unordered_map<std::string, std::shared_ptr<Mesh>> mesh_library;

    // all objects of the scene addressable by name
    std::unordered_map<std::string, Model> scene; 
};
