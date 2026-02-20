#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "app_imgui.hpp"
#include "app.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

AppImGui::AppImGui( App* a, GLFWwindow* w) {
    app = a;
    window = w;
}

void AppImGui::init() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup modern rounded design
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 3.0f;
    style.FrameRounding     = 3.0f;
    style.PopupRounding     = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding      = 3.0f;
    style.TabRounding       = 3.0f;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");
}

void AppImGui::new_frame() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void AppImGui::gui(double delta_time) {
    //Main gui implementaion
    if (imgui_open) {
        ImGui::SetNextWindowPos(ImVec2(24, 24));
        
        const ImGuiWindowFlags flags = 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_NoBackground | 
            ImGuiWindowFlags_NoTitleBar;

        ImGui::Begin("Main Menu", &imgui_open, flags); 
        {
            if (ImGui::Checkbox("VSync", &app->app_settings.vsync)) {
                glfwSwapInterval(app->app_settings.vsync);
            }
            ImGui::Checkbox("Debug Window", &debug_window_open);
        }
        ImGui::End();

        if (debug_window_open) {
            ImGui::ShowDemoWindow(&debug_window_open);
        }
    }
}

void AppImGui::render() {
    //Render
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool AppImGui::capture_mouse() {
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

bool AppImGui::capture_keyboard() {
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureKeyboard;
}

AppImGui::~AppImGui() {
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
