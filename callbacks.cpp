#include "app.hpp"

#include <iostream>
//#include <GL/glew.h> 
//#include <GL/wglew.h> 

#include <GLFW/glfw3.h>

#include "gl_err_callback.h"
#include "glfw_helpers.hpp"

void App::init_callbacks() {
    glfwSetKeyCallback(window, GlfwBinder<&App::key_callback>::callback);
    glfwSetFramebufferSizeCallback(window, GlfwBinder<&App::fbsize_callback>::callback);
    glfwSetWindowPosCallback(window, GlfwBinder<&App::window_pos_callback>::callback);
    glfwSetWindowMaximizeCallback(window, GlfwBinder<&App::window_maximization_callback>::callback);
    glfwSetMouseButtonCallback(window, GlfwBinder<&App::mouse_button_callback>::callback);
    glfwSetCursorPosCallback(window, GlfwBinder<&App::cursor_position_callback>::callback);
    glfwSetScrollCallback(window, GlfwBinder<&App::scroll_callback>::callback);
}

void App::key_callback(int key, int scancode, int action, int mods) {
    if ((action == GLFW_PRESS) || (action == GLFW_REPEAT)) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        default:
            break;
        }
    }

    if (imgui->capture_keyboard()) return;

    if ((action == GLFW_PRESS) || (action == GLFW_REPEAT)) {
        switch (key) {
        case GLFW_KEY_X:
            set_gui_enabled(!app_settings.gui_enabled);
            break;
        case GLFW_KEY_V:
            // Vsync on/off
            set_vsync(!app_settings.vsync);
            break;
        case GLFW_KEY_F:
            set_fullscreen(!app_settings.fullscreen);
            break;
        case GLFW_KEY_F11:
            if (screenshot != 0) return;
            screenshot = 1;
            break;
        default:
            break;
        }
    }
}
void App::fbsize_callback(int width, int height) {
    std::cout << "fbsize_callback: width " << width << ", height " << height << std::endl;

    //might differ but lets make it simple for now
    if (!app_settings.fullscreen && !app_settings.window_maximized) {
        app_settings.window_width = width;
        app_settings.window_height = height;
    }

    fb_width = width;
    fb_height = height;

    // set viewport
    glViewport(0, 0, width, height);

    //now your canvas has [0,0] in bottom left corner, and its size is [width x height] 

    this->update_projection_matrix();
}
int backup_window_pos_x = -1;
int backup_window_pos_y = -1;
void App::window_pos_callback(int xpos, int ypos) {
    std::cout << "window_pos_callback: xpos " << xpos << ", ypos " << ypos << std::endl;

    if (!app_settings.fullscreen && !app_settings.window_maximized) {
        backup_window_pos_x = app_settings.window_pos_x;
        backup_window_pos_y = app_settings.window_pos_y;
        app_settings.window_pos_x = xpos;
        app_settings.window_pos_y = ypos < 20 ? 20 : ypos; // give window titlebar some space
    }
}
void App::window_maximization_callback(int maximized) {
    std::cout << "window_maximization_callback: maximized " << maximized << std::endl;

    app_settings.window_maximized = maximized;
    if (maximized && backup_window_pos_x != -1) {
        app_settings.window_pos_x = backup_window_pos_x;
        app_settings.window_pos_y = backup_window_pos_y;
    }
}
void App::mouse_button_callback(int button, int action, int mods) {
    if (imgui->capture_mouse()) return;

    std::cout << "mouse_button_callback: button " << button << ", action " << action << ", mods " << mods << std::endl;
}
void App::cursor_position_callback(double xpos, double ypos) {
    if (app_settings.gui_enabled) {
        last_cursor_pos_x = xpos;
        last_cursor_pos_y = ypos;
        return;
    }

    camera.ProcessMouseMovement(xpos - last_cursor_pos_x, (ypos - last_cursor_pos_y) * -1.0);

    last_cursor_pos_x = xpos;
    last_cursor_pos_y = ypos;
    //std::cout << "cursor_position_callback: xpos " << xpos << ", ypos " << ypos << std::endl;
}
void App::scroll_callback(double xoffset, double yoffset) {
    if (imgui->capture_mouse()) return;

    this->fov += 10 * yoffset; // yoffset is mostly +1 or -1; one degree difference in fov is not visible
    this->fov = std::clamp(this->fov, 20.0f, 170.0f); // limit FOV to reasonable values...

    this->update_projection_matrix();
}
