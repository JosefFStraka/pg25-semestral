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
#include "image_io.hpp"
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

        // GLFW callbacks registration
        init_callbacks();

        init_imgui();

        // When all is loaded, show the window.
        glfwShowWindow(window);

        set_gui_enabled(app_settings.gui_enabled);
        set_fullscreen(app_settings.fullscreen);
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

    // multisampling
    glfwWindowHint(GLFW_SAMPLES, 4);

    // open window, but hidden - it will be enabled later, after asset initialization
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    if (app_settings.window_pos_x != -1 || app_settings.window_pos_y != -1) {
        glfwWindowHint(GLFW_POSITION_X, app_settings.window_pos_x);
        glfwWindowHint(GLFW_POSITION_Y, app_settings.window_pos_y);
    }

    if (app_settings.window_width <= 0) {
        app_settings.window_width = 800;
    }
    if (app_settings.window_height <= 0) {
        app_settings.window_height = 600;
    }

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(app_settings.window_width, app_settings.window_height, "ICP", nullptr, nullptr);
    if (!window) {
        throw std::runtime_error("GLFW window can not be created.");
    }

    glfwGetWindowPos(window, &app_settings.window_pos_x, &app_settings.window_pos_y);
    glfwGetWindowSize(window, &app_settings.window_width, &app_settings.window_height);
    saved_window_pos_x = app_settings.window_pos_x;
    saved_window_pos_y = app_settings.window_pos_y;
    saved_window_width = app_settings.window_width;
    saved_window_height = app_settings.window_height;

    glfwGetFramebufferSize(window, &fb_width, &fb_height);

    glfwSetWindowUserPointer(window, this);

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
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

void load_mesh(std::unordered_map<std::string, std::shared_ptr<Mesh>>& library, std::string filename, std::string modelname) {
    if (!std::filesystem::exists(filename)) {
        throw std::runtime_error("File does not exist: " + filename);
    } else {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        if (!loadOBJ(filename, vertices, indices)) {
            throw std::runtime_error("Loading failed: " + filename);
        }

        library.emplace(modelname, std::make_shared<Mesh>(vertices, indices, GL_TRIANGLES));
    }
}

ResourceHandle<ModelResource> createSimpleModel(
    ResourceManager& rm,
    ResourceHandle<Mesh> mesh,
    ResourceHandle<Texture> texture,
    ResourceHandle<ShaderProgram> shader) {
    ModelResource res;

    res.meshes.push_back({
        mesh,
        texture,
        shader,
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        glm::vec3(1.0f)
        });

    return rm.createModelResource(std::move(res));
}
ModelInstance createModelInstance(ResourceHandle<ModelResource> mR) {
    ModelInstance mI;
    mI.model = mR;
    return mI;
}

//MARK: App::init_assets
void App::init_assets(void) {

    // all shaders: load, compile, link, initialize params, place to library
    shader_library.emplace("skybox", resources.emplaceShader("../engine/assets/shaders/skybox.vert", "../engine/assets/shaders/skybox.frag", false));
    shader_library.emplace("phong", resources.emplaceShader("../resources/shaders/phong.vert", "../resources/shaders/phong.frag", false));

    //mesh library: meshes, that can be shared by multiple models
    std::vector<Vertex> line = { Vertex{.position = {0.f,0.f,0.f}},Vertex{.position = {1.f,0.f,0.f}} };
    mesh_library.emplace("line", resources.emplaceMesh(line, GL_LINES));
    std::vector<Vertex> triangle = { Vertex{.position = {-1.f, -1.f, 0.f}}, Vertex{.position = {3.f, -1.f, 0.f}}, Vertex{.position = {-1.f, 3.f, 0.f}} };
    mesh_library.emplace("skybox_triangle", resources.emplaceMesh(triangle, GL_TRIANGLES));

    mesh_library.emplace("plane", resources.registerMesh("../engine/assets/meshes/plane.obj"));
    mesh_library.emplace("cube", resources.registerMesh("../engine/assets/meshes/cube.obj"));
    mesh_library.emplace("sphere_tri_vnt", resources.registerMesh("../resources/assets/obj_samples/sphere_tri_vnt.obj"));
    mesh_library.emplace("triangle", resources.registerMesh("../resources/04/2d_obj_samples/triangle.obj"));
    mesh_library.emplace("teapot_tri_vnt", resources.registerMesh("../resources/teapot_tri_vnt.obj"));
    mesh_library.emplace("bunny", resources.registerMesh("../resources/pepa/bunny/bunny.obj"));
    mesh_library.emplace("dragon", resources.registerMesh("../resources/pepa/dragon/dragon.obj"));
    mesh_library.emplace("sponza", resources.registerMesh("../resources/pepa/sponza/sponza.obj"));

    texture_library.emplace("white", resources.emplaceTexture(glm::vec3(1.f, 1.f, 1.f)));
    texture_library.emplace("default", resources.registerTexture("../engine/assets/textures/default.png"));
    texture_library.emplace("wood_box", resources.registerTexture("../resources/textures/box_rgb888.png"));
    texture_library.emplace("TextureDouble_A", resources.registerTexture("../resources/textures/TextureDouble_A.png"));
    texture_library.emplace("vlada", resources.registerTexture("../resources/pepa/IMG_20231228_021715.jpg"));
    texture_library.emplace("widevojta", resources.registerTexture("../resources/pepa/widevojta.jpg"));

    auto vladaBallHandle = createSimpleModel(resources, mesh_library.at("sphere_tri_vnt"), texture_library.at("vlada"), shader_library.at("phong"));
    {
        ModelInstance vladaBall = createModelInstance(vladaBallHandle);
        vladaBall.setPosition(glm::vec3(-2.f, 0.f, 0.f));
        scene.models.emplace("vlada_ball", vladaBall);
    }
    auto vojtaBallHandle = createSimpleModel(resources, mesh_library.at("sphere_tri_vnt"), texture_library.at("widevojta"), shader_library.at("phong"));
    {
        ModelInstance slunce_nase_jasne = createModelInstance(vojtaBallHandle);
        slunce_nase_jasne.setPosition(glm::vec3(1.f, 2.f, 0.f));
        slunce_nase_jasne.setScale(glm::vec3(0.1f, 0.1f, 0.1f));
        scene.models.emplace("slunce_nase_jasne", slunce_nase_jasne);
    }
    {
        ModelInstance vojtaBall = createModelInstance(vojtaBallHandle);
        vojtaBall.setPosition(glm::vec3(0.f, 0.f, 0.f));
        scene.models.emplace("vojta_ball", vojtaBall);
    }
    auto boxHandle = createSimpleModel(resources, mesh_library.at("cube"), texture_library.at("wood_box"), shader_library.at("phong"));
    {
        ModelInstance m_box_texture = createModelInstance(boxHandle);
        m_box_texture.setPosition(glm::vec3(2.f, 0.f, 0.f));
        scene.models.emplace("m_box_texture", m_box_texture);
    }

    // Model m_triangle;
    // m_triangle.addMesh(mesh_library.at("triangle"), texture_library.at("white"), shader_library.at("simple_uniform_shader"));
    // // scene.models.emplace("m_triangle", m_triangle);

    // Model m_cube;
    // m_cube.addMesh(mesh_library.at("cube"), texture_library.at("white"), shader_library.at("rainbow"));
    // // scene.models.emplace("m_cube", m_cube);

    // Model m_teapot;
    // m_teapot.addMesh(mesh_library.at("teapot_tri_vnt"), texture_library.at("default"), shader_library.at("tex"));
    // m_teapot.setScale(glm::vec3(0.1f, 0.1f, 0.1f));
    // //scene.models.emplace("m_teapot", m_teapot);

    // // bigger moddels, takes longer to load


    auto bunnyHandle = createSimpleModel(resources, mesh_library.at("bunny"), texture_library.at("TextureDouble_A"), shader_library.at("phong"));
    {
        ModelInstance bunny = createModelInstance(bunnyHandle);
        bunny.setPosition(glm::vec3(0.2f, -0.5f, 0.f));
        bunny.setEulerAngles(glm::vec3(0.f, 335.f, 0.f));
        bunny.setScale(glm::vec3(0.8f, 0.8f, 0.8f));
        scene.models.emplace("bunny", bunny);
    }

    auto dragonHandle = createSimpleModel(resources, mesh_library.at("dragon"), texture_library.at("default"), shader_library.at("phong"));
    {
        ModelInstance dragon = createModelInstance(dragonHandle);
        dragon.setScale(glm::vec3(1.8f, 1.8f, 1.8f));
        //scene.models.emplace("dragon", dragon);
    }

    auto sponzaHandle = createSimpleModel(resources, mesh_library.at("sponza"), texture_library.at("default"), shader_library.at("phong"));
    {
        ModelInstance sponza = createModelInstance(sponzaHandle);
        sponza.setScale(glm::vec3(0.01f, 0.01f, 0.01f));
        //scene.models.emplace("sponza", sponza);
    }

    std::string base_path = "../resources/textures/skyboxes/vylety_20260403_cubemap/";
    CubeMapTexture* cm = new CubeMapTexture({
        base_path + "_px.png",
        base_path + "_nx.png",
        base_path + "_py.png",
        base_path + "_ny.png",
        base_path + "_pz.png",
        base_path + "_nz.png",
        });

    scene.skybox = new Skybox(mesh_library.at("skybox_triangle"), cm, shader_library.at("skybox"));

    axis_display.init();
    axis_display.set_viewport(0, 0, 64, 64);
}

// MARK: RUN
int App::run() {
    try {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        glViewport(0, 0, fb_width, fb_height);
        update_projection_matrix();


        glfwGetCursorPos(window, &last_cursor_pos_x, &last_cursor_pos_y);

        camera.Position = glm::vec3(0.0f, 0.0f, 4.0f);

        float rotation_speed = 0.f;

        glClearColor(0.2f, 0.2f, 0.2f, 1);
        double last_time = -1 / 60.0;
        double last_fps_time = 0.0;

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
                    ImGui::Text("FPS: %4.1f %4.0f %4.0f ", FPS.get_current(), FPS.get_1_low(), FPS.get_01_low());
                    if (ImGui::Checkbox("VSync", &this->app_settings.vsync)) {
                        set_vsync(this->app_settings.vsync);
                    }
                    if (ImGui::Checkbox("Antialiasing (MSAA 4x)", &this->app_settings.msaa_enabled)) {
                        set_msaa(this->app_settings.msaa_enabled);
                    }
                    if (ImGui::Checkbox("Fullscreen", &this->app_settings.fullscreen)) {
                        set_fullscreen(app_settings.fullscreen);
                    }
                    ImGui::Checkbox("Axis display", &this->app_settings.gui_axis_display_enabled);
                    ImGui::Checkbox("GUI always active", &this->app_settings.gui_always_enabled);

                    ImGui::Separator();

                    ImGui::SliderFloat("Rotation speed", &rotation_speed, 0.f, 10.f);
                    if (ImGui::SliderFloat("FoV", &this->fov, 20.f, 180.f)) {
                        update_projection_matrix();
                    }
                    ImGui::Checkbox("Demo Window", &imgui->debug_window_open);

                    ImGui::Text("Camera:");
                    ImGui::Text("x: %.2f | y: %.2f | z: %.2f", camera.Position.x, camera.Position.y, camera.Position.z);
                    ImGui::Text("pitch: %.1f | yaw: %.1f", camera.Pitch, camera.Yaw);

                    if (ImGui::TreeNode("Scene")) {
                        size_t i = 0;
                        for (auto& [name, model] : scene.models) {
                            {
                                ImGui::PushID(i);
                                if (ImGui::TreeNode("", name.c_str())) {
                                    AppImGui::model_controls(&model);
                                    ImGui::TreePop();
                                }
                                ImGui::PopID();
                                i++;
                            }
                        }
                        ImGui::TreePop();
                    }
                }

                if (app_settings.gui_always_enabled && !app_settings.gui_enabled) {
                    ImGui::EndDisabled();
                }
                imgui->gui_end();
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            auto now = glfwGetTime();
            double delta_time = now - last_time;
            last_time = now;

            //########## react to user  ##########
            camera.Position += camera.ProcessInput(window, delta_time); // process keys etc.

            // HSL data = HSL((int)(now * (360 / 5)) % 360, 1.f, 0.5f);
            // RGB value = HSLToRGB(data);
            // resources.getShader(simple_uniform_shader)->setUniform("ucolor", glm::vec4(value.R / 255.0, value.G / 255.0, value.B / 255.0, 1.f));

            // resources.getShader(rainbow_shader)->setUniform("iTime", (float)now);

            //########## create and set View Matrix according to camera settings  ##########
            for (auto& [_, shaderHandle] : shader_library) {
                auto shader = resources.getShader(shaderHandle);
                shader->setUniform("uV_m", camera.GetViewMatrix());
                shader->setUniform("uP_m", projection_matrix);
            }

            for (auto&& [_, model] : scene.models) {
                model.rotate(glm::vec3(17.f * rotation_speed * delta_time, 31.f * rotation_speed * delta_time, 11.f * rotation_speed * delta_time));
                model.update(delta_time);
            }

            renderer.render(&resources, &scene);

            if (app_settings.gui_axis_display_enabled)
                axis_display.draw(camera);

            if (should_draw_gui) {
                imgui->render();
            }

            glfwSwapBuffers(window);

            if (screenshot != 0) {
                static int saved_msaa = -1;
                std::string ss = app_settings.msaa_enabled == 1 ? "msaa.png" : "nomsaa.png";
                take_screenshot(ss);
                if (screenshot == 2) {
                    screenshot = 0;
                    set_msaa(saved_msaa);
                } else {
                    screenshot++;
                    saved_msaa = app_settings.msaa_enabled;
                    set_msaa(!app_settings.msaa_enabled);
                }
            }

            glfwPollEvents();

            FPS.update();

            if (now - last_fps_time > 1.0) {
                last_fps_time = now;
                std::cout << std::format("FPS: {:6.1f} | 1%: {:3.0f} | 0.1%: {:3.0f}", FPS.get_current(), FPS.get_1_low(), FPS.get_01_low()) << std::endl;
            }
        }
    }
    catch (std::exception const& e) {
        std::cerr << "App failed : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
//MARK: END RUN

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
        if (app_settings.fullscreen)
            ImGui::GetIO().MouseDrawCursor = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
        ImGui::GetIO().MouseDrawCursor = false;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    std::cout << "Fullscreen: " << app_settings.fullscreen << "\n";
}

void App::set_gui_enabled(bool value) {
    app_settings.gui_enabled = value;

    if (app_settings.gui_enabled) {
        if (app_settings.fullscreen)
            ImGui::GetIO().MouseDrawCursor = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
        ImGui::GetIO().MouseDrawCursor = false;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    std::cout << "GUI: " << app_settings.gui_enabled << "\n";
}

void App::set_vsync(bool value) {
    app_settings.vsync = value;
    glfwSwapInterval(app_settings.vsync);
    std::cout << "VSync: " << app_settings.vsync << "\n";
}

void App::set_msaa(bool value) {
    app_settings.msaa_enabled = value;

    if (app_settings.msaa_enabled) {
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_MULTISAMPLE);
    }

    std::cout << "MSAA: " << app_settings.msaa_enabled << "\n";
}

void App::take_screenshot(std::string path) {
    GLMat framebuffer(fb_height, fb_width, GL_8UC4);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, fb_width, fb_height, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer.data());

    if (imwrite(path, framebuffer, 1) == 0) {
        std::cout << "Failed to save screenshot! (path:\"" << path << "\")" << std::endl;
    }
}

void App::update_projection_matrix(void) {
    if (fb_height < 1)
        fb_height = 1;   // avoid division by 0

    float ratio = static_cast<float>(fb_width) / fb_height;

    projection_matrix = glm::perspective(
        glm::radians(fov),   // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90� (extra wide) and 30� (quite zoomed in)
        ratio,               // Aspect Ratio. Depends on the size of your window.
        0.1f,                // Near clipping plane. Keep as big as possible, or you'll get precision issues.
        20000.0f             // Far clipping plane. Keep as little as possible.
    );

    axis_display.viewport[2] = (GLsizei)std::floorf(fb_width / 15.f);
    axis_display.viewport[3] = (GLsizei)std::floorf(fb_width / 15.f);
}

App::~App() {
    settings::save("settings.json", app_settings);

    if (scene.skybox) delete scene.skybox;

    delete imgui;
}
