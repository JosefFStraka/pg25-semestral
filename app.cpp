#include "app.hpp"

#include <iostream>
#include <chrono>
#include <stack>
#include <random>
#include <format>

#include <GL/glew.h>
#include <GL/wglew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "gl_err_callback.h"
#include "glfw_helpers.hpp"

#include "imgui.h"
#include "meshgen.hpp"

//---------------------------------------------------------------------

App::App() {
    // default constructor
    // nothing to do here (so far...)
    std::cout << "Constructed...\n";
}

// MARK: INIT
bool App::init() {
    try {
        std::cout << "Current working directory: " << std::filesystem::current_path().generic_string() << '\n';

        if (settings::load("settings.json", app_settings)) {
            std::cout << "Settings file settings.json doesnt exist" << std::endl;
        }

        if (!std::filesystem::exists("../resources"))
            throw std::runtime_error("Directory 'resources' not found. Various media files are expected to be there.");

        // init_opencv();
        init_glfw();
        init_glew();
        init_gl_debug();

        // print_opencv_info();
        // print_glfw_info();
        print_gl_info();
        // print_glm_info();

        glfwSwapInterval(app_settings.vsync);

        init_assets();

        init_callbacks();

        init_imgui();

        // When all is loaded, show the window.
        glfwShowWindow(window);
    }
    catch (std::exception const& e) {
        std::cerr << "Init failed : " << e.what() << std::endl;
        throw;
    }

    return true;
}

void App::init_imgui(void) {
    imgui = new AppImGui(window);
    imgui->init();
    std::cout << "ImGUI version: " << ImGui::GetVersion() << "\n";
}

void App::init_glfw(void) {

    /* Initialize the library */
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        throw std::runtime_error("GLFW can not be initialized.");
    }


    // try to open OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // open window, but hidden - it will be enabled later, after asset initialization
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    if (app_settings.window_pos_x != -1 || app_settings.window_pos_y != -1) {
        glfwWindowHint(GLFW_POSITION_X, app_settings.window_pos_x);
        glfwWindowHint(GLFW_POSITION_Y, app_settings.window_pos_y);
    }

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(800, 600, "ICP", nullptr, nullptr);
    if (!window) {
        throw std::runtime_error("GLFW window can not be created.");
    }

    glfwSetWindowUserPointer(window, this);

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    // disable mouse cursor
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLFW callbacks registration
    init_callbacks();
}

void App::init_glew() {
    // init glew
    // http://glew.sourceforge.net/basic.html
    GLenum err;
    err = glewInit();
    GLEW_CHECK(err, "glewInit");
    err = wglewInit();
    GLEW_CHECK(err, "wglewInit");

    if (!GLEW_ARB_direct_state_access)
        throw std::runtime_error("No DSA :-(");
}

void App::init_gl_debug(void) {
    if (GLEW_ARB_debug_output) {
        glDebugMessageCallback(MessageCallback, 0);
        glEnable(GL_DEBUG_OUTPUT);

#ifndef DEBUG
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
#endif

        // default is asynchronous debug output, use this to simulate glGetError() functionality
        // glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

        std::cout << "GL_DEBUG enabled." << std::endl;
    } else {
        std::cout << "GL_DEBUG NOT SUPPORTED!" << std::endl;
    }
}

void App::init_assets(void) {

    // all shaders: load, compile, link, initialize params, place to library
    shader_library.emplace("simple_shader", std::make_shared<ShaderProgram>("../resources/basic_core.vert", "../resources/basic_core.frag", false));
    shader_library.emplace("simple_uniform_shader", std::make_shared<ShaderProgram>("../resources/basic_core.vert", "../resources/basic_uniform.frag", false));
    shader_library.emplace("rainbow", std::make_shared<ShaderProgram>("../resources/basic_core.vert", "../resources/rainbow.frag", false));

    //mesh library: meshes, that can be shared by multiple models
    mesh_library.emplace("sphere_lowpoly", std::make_shared<Mesh>(generateCube()));
    mesh_library.emplace("sphere_lowpoly", std::make_shared<Mesh>(generateSphere(4, 4)));
    mesh_library.emplace("sphere_highpoly", std::make_shared<Mesh>(generateSphere(8, 8)));

    // load mesh from .OBJ
    {
        std::filesystem::path filename = "../resources/04/2d_obj_samples/triangle.obj"; // or loaded from JSON etc...

        if (!std::filesystem::exists(filename)) {
            throw std::runtime_error("File does not exist: " + filename.string());
        } else {
            std::vector<Vertex> vertices;
            std::vector<GLuint> indices;
            if (!loadOBJ(filename, vertices, indices)) {
                throw std::runtime_error("Loading failed: " + filename.string());
            }

            mesh_library.emplace("triangle", std::make_shared<Mesh>(vertices, indices, GL_TRIANGLES));
        }
    }
    {
        std::filesystem::path filename = "../resources/teapot_tri_vnt.obj"; // or loaded from JSON etc...

        if (!std::filesystem::exists(filename)) {
            throw std::runtime_error("File does not exist: " + filename.string());
        } else {
            std::vector<Vertex> vertices;
            std::vector<GLuint> indices;
            if (!loadOBJ(filename, vertices, indices)) {
                throw std::runtime_error("Loading failed: " + filename.string());
            }

            mesh_library.emplace("teapot_tri_vnt", std::make_shared<Mesh>(vertices, indices, GL_TRIANGLES));
        }
    }

    // model: load model file, assign shader used to draw a model, put to scene
    // Model my_model = Model("resources/objects/hierarchical.obj", shader_library.at("simple_shader"));
    // scene.emplace("my_first_object", my_model);

    Model m_triangle;
    m_triangle.addMesh(mesh_library.at("triangle"), shader_library.at("simple_uniform_shader"));
    //scene.emplace("m_triangle", m_triangle);

    Model m_teapot;
    m_teapot.addMesh(mesh_library.at("teapot_tri_vnt"), shader_library.at("simple_uniform_shader"));
    m_teapot.setScale(glm::vec3(0.1f, 0.1f, 0.1f));
    scene.emplace("m_teapot", m_teapot);

    // Model m_cube;
    // m_cube.addMesh(mesh_library.at("cube"), shader_library.at("simple_shader"));//shader_library.at("simple_uniform_shader"));
    // scene.emplace("m_cube", m_cube);

    // reuse mesh and shader data to construct complex model
    //Model m;
    //m.addMesh(mesh_library.at("cube"), shader_library.at("simple_shader"));//shader_library.at("simple_uniform_shader"));
    // m.addMesh(mesh_library.at("sphere_lowpoly"), shader_library.at("simple_shader"));
    //m.addMesh(mesh_library.at("loadedFromFile"), shader_library.at("rainbow"));
    //scene.emplace("my_complex_object", m);
}

// MARK: RUN
int App::run() {
    try {
        /* Typical game loop:

                // INIT: Initial positions and state
                while (application_should_not_close)
                {
                    // UPDATE: Update game state
                    // RENDER: Render content
                    // SWAP: Swap back/front buffer
                    // VSYNC: Wait for vertical retrace (e.g. 1/60 of a second)
                    // POLL: Poll events, dispatch
                }
        */

        update_projection_matrix();
        glViewport(0, 0, width, height);

        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        glfwGetCursorPos(window, &last_cursor_pos_x, &last_cursor_pos_y);

        camera.Position = glm::vec3(0.0f, 0.0f, 5.0f);


        auto simple_uniform_shader = shader_library.at("simple_uniform_shader"); // crated a copy of shared pointer. Shader is guaranteed to live.
        auto rainbow_shader = shader_library.at("rainbow"); // crated a copy of shared pointer. Shader is guaranteed to live.

        glClearColor(0, 0, 0, 1);
        double last_time = -1 / 60.0;

        while (!glfwWindowShouldClose(window)) {

            bool should_draw_gui = app_settings.gui_enabled || app_settings.gui_always_enabled;
            // ImGui prepare render (only if required)
            if (should_draw_gui) {

                imgui->new_frame();
                imgui->gui_begin();
                if (app_settings.gui_always_enabled && !app_settings.gui_enabled) {
                    ImGui::BeginDisabled();
                }
                {
                    ImGui::Text("FPS: %.1f (%.1f - %.1f)", FPS.get_current(), FPS.get_min(), FPS.get_max());
                    if (ImGui::Checkbox("VSync", &this->app_settings.vsync)) {
                        set_vsync(this->app_settings.vsync);
                    }
                    if (ImGui::Checkbox("Fullscreen", &this->app_settings.fullscreen)) {
                        set_fullscreen(app_settings.fullscreen);
                    }
                    ImGui::Checkbox("GUI always active", &this->app_settings.gui_always_enabled);

                    ImGui::Checkbox("Debug Window", &imgui->debug_window_open);

                    if (ImGui::CollapsingHeader("Scene")) {
                        size_t i = 0;
                        for (auto const& pair : scene) {
                            {
                                // Here we use PushID() to generate a unique base ID, and then the "" used as TreeNode id won't conflict.
                                // An alternative to using 'PushID() + TreeNode("", ...)' to generate a unique ID is to use 'TreeNode((void*)(intptr_t)i, ...)',
                                // aka generate a dummy pointer-sized value to be hashed. The demo below uses that technique. Both are fine.
                                ImGui::PushID(i);
                                auto const name = pair.first;
                                auto const model = pair.second;
                                if (ImGui::TreeNode("", name.c_str())) {
                                    ImGui::Text("pivot_position: {%d %d %d}", model.pivot_position.x, model.pivot_position.y, model.pivot_position.z);
                                    ImGui::Text("eulerAngles: {%d %d %d}", model.eulerAngles.x, model.eulerAngles.y, model.eulerAngles.z);
                                    ImGui::Text("scale: {%d %d %d}", model.scale.x, model.scale.y, model.scale.z);
                                    ImGui::Text("mashes: %d", model.meshes.size());
                                    ImGui::TreePop();
                                }
                                ImGui::PopID();
                                i++;
                            }

                            // for (auto const& pair : scene) {
                            //     ImGui::BulletText();
                            // }
                        }
                    }
                }

                if (app_settings.gui_always_enabled && !app_settings.gui_enabled) {
                    ImGui::EndDisabled();
                }
                imgui->gui_end();
            }

            //
            // UPDATE: recompute objects state, players position etc.
            //         nothing here so far...
            //



            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            auto now = glfwGetTime();
            double delta = now - last_time;
            last_time = now;

            //########## react to user  ##########
            camera.Position += camera.ProcessInput(window, delta); // process keys etc.

            //########## create and set View Matrix according to camera settings  ##########
            simple_uniform_shader->setUniform("uV_m", camera.GetViewMatrix());
            simple_uniform_shader->setUniform("uP_m", projection_matrix);

            HSL data = HSL((int)(now * (360 / 5)) % 360, 1.f, 0.5f);
            RGB value = HSLToRGB(data);
            simple_uniform_shader->setUniform("ucolor", glm::vec4(value.R / 255.0, value.G / 255.0, value.B / 255.0, 1.f));

            rainbow_shader->setUniform("iTime", (float)now);



            for (auto const& pair : scene) {
                Model model = pair.second;
                model.update(delta);
                model.draw();
            }

            // ImGui display
            if (should_draw_gui) {
                imgui->render();
            }

            glfwSwapBuffers(window);

            glfwPollEvents();

            // if (FPS.is_updated()) { // display new value only once per interval (default = 1.0s)
            //     std::cout << "FPS: " << FPS.get_current() << " (min: " << FPS.get_min() << ", max: " << FPS.get_max() << ")" << std::endl;
            // }

            FPS.update();
        }
    }
    catch (std::exception const& e) {
        std::cerr << "App failed : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void App::print_gl_info() {

    GL_PRINT_STRING(GL_VENDOR);
    GL_PRINT_STRING(GL_RENDERER);
    GL_PRINT_STRING(GL_VERSION);
    GL_PRINT_STRING(GL_SHADING_LANGUAGE_VERSION);

    GL_PRINT_NUMBER(GL_MAJOR_VERSION);
    GL_PRINT_NUMBER(GL_MINOR_VERSION);

    GLint myint;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &myint);

    if (myint & GL_CONTEXT_CORE_PROFILE_BIT) {
        std::cout << "We are using CORE profile\n";
    } else if (myint & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) {
        std::cout << "We are using COMPATIBILITY profile\n";
    } else {
        // throw std::runtime_error("What??");
    }

    glGetIntegerv(GL_CONTEXT_FLAGS, &myint);
    GL_PRINT_FLAG(myint, GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT);
    GL_PRINT_FLAG(myint, GL_CONTEXT_FLAG_DEBUG_BIT);
    GL_PRINT_FLAG(myint, GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT);
    GL_PRINT_FLAG(myint, GL_CONTEXT_FLAG_NO_ERROR_BIT);
}

void App::error_callback(int error, const char* description) {
    std::cerr << "Error: " << description << std::endl;
}

int saved_refresh_rate = -1;
void App::set_fullscreen(bool value) {
    app_settings.fullscreen = value;

    if (app_settings.fullscreen) {
        auto monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        saved_window_pos_x = app_settings.window_pos_x;
        saved_window_pos_y = app_settings.window_pos_y;
        saved_window_width = app_settings.window_width;
        saved_window_height = app_settings.window_height;

        saved_refresh_rate = mode->refreshRate;

        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        glfwSetWindowMonitor(window, NULL, saved_window_pos_x, saved_window_pos_y, saved_window_width, saved_window_height, NULL);
    }

    if (app_settings.gui_enabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    std::cout << "Fullscreen: " << app_settings.fullscreen << "\n";
}

void App::set_gui_enabled(bool value) {
    app_settings.gui_enabled = value;

    if (app_settings.gui_enabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    std::cout << "GUI: " << app_settings.gui_enabled << "\n";
}

void App::set_vsync(bool value) {
    app_settings.vsync = value;
    glfwSwapInterval(app_settings.vsync);
    std::cout << "VSync: " << app_settings.vsync << "\n";
}

void App::update_projection_matrix(void) {
    if (height < 1)
        height = 1;   // avoid division by 0

    float ratio = static_cast<float>(width) / height;

    projection_matrix = glm::perspective(
        glm::radians(fov),   // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90� (extra wide) and 30� (quite zoomed in)
        ratio,               // Aspect Ratio. Depends on the size of your window.
        0.1f,                // Near clipping plane. Keep as big as possible, or you'll get precision issues.
        20000.0f             // Far clipping plane. Keep as little as possible.
    );
}

App::~App() {
    settings::save("settings.json", app_settings);

    delete imgui;
}
