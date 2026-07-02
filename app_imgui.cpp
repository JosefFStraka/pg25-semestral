#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "app_imgui.hpp"
#include "app.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

AppImGui::AppImGui(GLFWwindow* w) {
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
    style.WindowRounding = 3.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;

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

void AppImGui::gui_begin() {
    ImGui::SetNextWindowPos(ImVec2(24, 24));
    ImGui::SetNextWindowSize(ImVec2(0.f, 0.f)); //autofit

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        // ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoTitleBar;

    ImGui::Begin("Main Menu", nullptr, flags);
}

void AppImGui::gui_end() {
    ImGui::End();

    if (debug_window_open) {
        ImGui::ShowDemoWindow(&debug_window_open);
    }
}

void AppImGui::render() {
    //Render
    ImGui::Render();
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

void AppImGui::model_controls(ModelInstance* const model, bool* enabled, bool is_static) {
    bool modified = false;

    ImGui::Checkbox("Enabled", enabled);
    ImGui::Checkbox("Transparent", &model->is_transparent);
    ImGui::Checkbox("Collision", &model->collision_enabled);

    if (is_static) {
        ImGui::BeginDisabled();
    }

    ImGui::Text("Position:");
    ImGui::PushID(0);
    modified |= ImGui::SliderFloat("x", &model->pivot_position.x, -10.f, 10.f);
    modified |= ImGui::SliderFloat("y", &model->pivot_position.y, -10.f, 10.f);
    modified |= ImGui::SliderFloat("z", &model->pivot_position.z, -10.f, 10.f);
    ImGui::PopID();

    ImGui::PushID(1);
    ImGui::Text("Rotation:");
    modified |= ImGui::SliderFloat("x", &model->eulerAngles.x, 0.f, 360.f);
    modified |= ImGui::SliderFloat("y", &model->eulerAngles.y, 0.f, 360.f);
    modified |= ImGui::SliderFloat("z", &model->eulerAngles.z, 0.f, 360.f);
    ImGui::PopID();

    ImGui::PushID(2);
    ImGui::Text("Scale:");
    modified |= ImGui::SliderFloat("x", &model->scale.x, -2.f, 2.f);
    modified |= ImGui::SliderFloat("y", &model->scale.y, -2.f, 2.f);
    modified |= ImGui::SliderFloat("z", &model->scale.z, -2.f, 2.f);
    ImGui::PopID();

    if (is_static) {
        ImGui::EndDisabled();
    }

    if (modified) {
        model->parameters_modified = true;
    }
}

void AppImGui::light_controls(s_lights * lights, int i) {
    bool modified = false;
    ImGui::SliderFloat("x", &lights->position[i].x, -10.f, 10.f);
    ImGui::SliderFloat("y", &lights->position[i].y, -10.f, 10.f);
    ImGui::SliderFloat("z", &lights->position[i].z, -10.f, 10.f);
    ImGui::SliderFloat("w", &lights->position[i].w, 0.f, 1.f);
    ImGui::SliderFloat("dir X", &lights->direction[i].x, -1.f, 1.f);
    ImGui::SliderFloat("dir Y", &lights->direction[i].y, -1.f, 1.f);
    ImGui::SliderFloat("dir Z", &lights->direction[i].z, -1.f, 1.f);
    ImGui::ColorPicker3("Color", &lights->color[i].r);
    ImGui::SliderFloat("Attenuation", &lights->attenuation[i], 0.f, 1.f);
    ImGui::SliderFloat("spotCutoff", &lights->spotCutoff[i], 0.f, 180.f);
}
