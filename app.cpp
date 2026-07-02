#include "app.hpp"

#include <chrono>
#include <format>
#include <iostream>
#include <random>
#include <stack>


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

#undef min

const char* ShaderDebugOptions[]{ "None",    "NormalsVS", "Something", "Ambient",
                                 "Diffuse", "Specular",  "Light" };

//---------------------------------------------------------------------

App::App() {
    // default constructor
    // nothing to do here (so far...)
    std::cout << "Constructed...\n";
}

// MARK: INIT
bool App::init() {
    try {
        std::cout << "Current working directory: "
            << std::filesystem::current_path().generic_string() << '\n';

        if (settings::load("settings.json", app_settings)) {
            std::cout << "Settings file settings.json doesnt exist" << std::endl;
        }

        if (!std::filesystem::exists("../resources"))
            throw std::runtime_error("Directory '../resources' not found. Various "
                "media files are expected to be there.");

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

    // open window, but hidden - it will be enabled later, after asset
    // initialization
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

    if (app_settings.window_maximized) {
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    }

    /* Create a windowed mode window and its OpenGL context */
    window =
        glfwCreateWindow(app_settings.window_width, app_settings.window_height,
            "ICP", nullptr, nullptr);
    if (!window) {
        throw std::runtime_error("GLFW window can not be created.");
    }

    if (app_settings.window_maximized) {
        glfwGetWindowPos(window, &app_settings.window_pos_x,
            &app_settings.window_pos_y);
        glfwGetWindowSize(window, &app_settings.window_width,
            &app_settings.window_height);
        saved_window_pos_x = app_settings.window_pos_x;
        saved_window_pos_y = app_settings.window_pos_y;
        saved_window_width = app_settings.window_width;
        saved_window_height = app_settings.window_height;
    }

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
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
            GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
#endif

        // default is asynchronous debug output, use this to simulate glGetError()
        // functionality glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

        std::cout << "GL_DEBUG enabled." << std::endl;
    } else {
        std::cout << "GL_DEBUG NOT SUPPORTED!" << std::endl;
    }
}

ResourceHandle<ModelResource>
createSimpleModel(AssetManager& am, const std::string& name,
    ResourceHandle<Mesh> mesh, ResourceHandle<Texture> texture,
    ResourceHandle<ShaderProgram> shader) {
    ModelResource res;
    res.addMesh(mesh, texture, shader);
    return am.emplaceModel(name, std::move(res));
}
ModelInstance createModelInstance(ResourceHandle<ModelResource> mR) {
    ModelInstance mI;
    mI.model = mR;
    return mI;
}

// MARK: App::init_assets
void App::init_assets(void) {
    assets.load("../assets.json");

    current_scene = std::make_shared<Scene>();

    std::vector<Vertex> line = { Vertex{.position = {0.f, 0.f, 0.f}},
                                Vertex{.position = {1.f, 0.f, 0.f}} };
    assets.addProgrammatic("line", std::make_unique<Mesh>(line, GL_TRIANGLES));

    std::vector<Vertex> triangle = { Vertex{.position = {-1.f, -1.f, 0.f}},
                                    Vertex{.position = {3.f, -1.f, 0.f}},
                                    Vertex{.position = {-1.f, 3.f, 0.f}} };
    assets.addProgrammatic("skybox_triangle",
        std::make_unique<Mesh>(triangle, GL_TRIANGLES));

    std::vector<Vertex> aabb_lines_vertices = {
        Vertex{.position = {-0.5f, -0.5f, -0.5f}},
        Vertex{.position = {0.5f, -0.5f, -0.5f}},
        Vertex{.position = {0.5f, -0.5f, -0.5f}},
        Vertex{.position = {0.5f, 0.5f, -0.5f}},
        Vertex{.position = {0.5f, 0.5f, -0.5f}},
        Vertex{.position = {-0.5f, 0.5f, -0.5f}},
        Vertex{.position = {-0.5f, 0.5f, -0.5f}},
        Vertex{.position = {-0.5f, -0.5f, -0.5f}},

        Vertex{.position = {-0.5f, -0.5f, 0.5f}},
        Vertex{.position = {0.5f, -0.5f, 0.5f}},
        Vertex{.position = {0.5f, -0.5f, 0.5f}},
        Vertex{.position = {0.5f, 0.5f, 0.5f}},
        Vertex{.position = {0.5f, 0.5f, 0.5f}},
        Vertex{.position = {-0.5f, 0.5f, 0.5f}},
        Vertex{.position = {-0.5f, 0.5f, 0.5f}},
        Vertex{.position = {-0.5f, -0.5f, 0.5f}},

        Vertex{.position = {-0.5f, -0.5f, -0.5f}},
        Vertex{.position = {-0.5f, -0.5f, 0.5f}},
        Vertex{.position = {0.5f, -0.5f, -0.5f}},
        Vertex{.position = {0.5f, -0.5f, 0.5f}},
        Vertex{.position = {0.5f, 0.5f, -0.5f}},
        Vertex{.position = {0.5f, 0.5f, 0.5f}},
        Vertex{.position = {-0.5f, 0.5f, -0.5f}},
        Vertex{.position = {-0.5f, 0.5f, 0.5f}},
    };
    assets.addProgrammatic("aabb_lines",
        std::make_unique<Mesh>(aabb_lines_vertices, GL_LINES));

    std::vector<Vertex> ndc_cube_vertices = {
        Vertex{.position = {-1.f, -1.f, -1.f}},
        Vertex{.position = {1.f, -1.f, -1.f}},
        Vertex{.position = {1.f, -1.f, -1.f}},
        Vertex{.position = {1.f, 1.f, -1.f}},
        Vertex{.position = {1.f, 1.f, -1.f}},
        Vertex{.position = {-1.f, 1.f, -1.f}},
        Vertex{.position = {-1.f, 1.f, -1.f}},
        Vertex{.position = {-1.f, -1.f, -1.f}},

        Vertex{.position = {-1.f, -1.f, 1.f}},
        Vertex{.position = {1.f, -1.f, 1.f}},
        Vertex{.position = {1.f, -1.f, 1.f}},
        Vertex{.position = {1.f, 1.f, 1.f}},
        Vertex{.position = {1.f, 1.f, 1.f}},
        Vertex{.position = {-1.f, 1.f, 1.f}},
        Vertex{.position = {-1.f, 1.f, 1.f}},
        Vertex{.position = {-1.f, -1.f, 1.f}},

        Vertex{.position = {-1.f, -1.f, -1.f}},
        Vertex{.position = {-1.f, -1.f, 1.f}},
        Vertex{.position = {1.f, -1.f, -1.f}},
        Vertex{.position = {1.f, -1.f, 1.f}},
        Vertex{.position = {1.f, 1.f, -1.f}},
        Vertex{.position = {1.f, 1.f, 1.f}},
        Vertex{.position = {-1.f, 1.f, -1.f}},
        Vertex{.position = {-1.f, 1.f, 1.f}},
    };
    assets.addProgrammatic("ndc_lines",
        std::make_unique<Mesh>(ndc_cube_vertices, GL_LINES));

    assets.addProgrammatic("white",
        std::make_unique<Texture>(glm::vec3(1.f, 1.f, 1.f)));

    assets.addProgrammatic("debug_shader", std::make_unique<ShaderProgram>("../engine/assets/shaders/tex.vert", "../engine/assets/shaders/tex.frag", false));
    assets.addProgrammatic("color_shader",
        std::make_unique<ShaderProgram>(
            "../engine/assets/shaders/color.vert",
            "../engine/assets/shaders/color.frag", false));

    auto argus1Handle = createSimpleModel(assets, "argus1_model", assets.getHandle<Mesh>("argus1"), assets.getHandle<Texture>("argus1"), assets.getHandle<ShaderProgram>("phong"));
    {
        ModelInstance argus1 = createModelInstance(argus1Handle);
        argus1.setPosition(glm::vec3(-6.f, 0.f, 0.f));
        current_scene->add_static_model("argus1", std::move(argus1), assets);
    }

    auto teapotHandle = createSimpleModel(
        assets, "teapot_model", assets.getHandle<Mesh>("teapot_tri_vnt"),
        assets.getHandle<Texture>("TextureDouble_A"),
        assets.getHandle<ShaderProgram>("phong"));
    {
        ModelInstance teapot = createModelInstance(teapotHandle);
        teapot.setPosition(glm::vec3(-4.f, 0.f, 0.f));
        teapot.setScale(glm::vec3(0.1f, 0.1f, 0.1f));
        teapot.opacity = 0.7f;
        current_scene->add_static_model("teapot", std::move(teapot), assets);
    }

    auto vladaBallHandle = createSimpleModel(
        assets, "vladaBall_model", assets.getHandle<Mesh>("sphere_tri_vnt"),
        assets.getHandle<Texture>("vlada"),
        assets.getHandle<ShaderProgram>("chaos"));
    {
        ModelInstance vladaBall = createModelInstance(vladaBallHandle);
        vladaBall.setPosition(glm::vec3(-2.f, 0.f, 0.f));
        current_scene->add_model("vlada_ball", std::move(vladaBall));
    }
    auto vojtaBallHandle = createSimpleModel(
        assets, "vojtaBall_model", assets.getHandle<Mesh>("sphere_tri_vnt"),
        assets.getHandle<Texture>("widevojta"),
        assets.getHandle<ShaderProgram>("phong"));
    {
        ModelInstance slunce_nase_jasne = createModelInstance(vojtaBallHandle);
        slunce_nase_jasne.setPosition(glm::vec3(1.f, 2.f, 0.f));
        slunce_nase_jasne.setScale(glm::vec3(0.1f, 0.1f, 0.1f));
        current_scene->add_model("slunce_nase_jasne", std::move(slunce_nase_jasne));
    }
    {
        ModelInstance vojtaBall = createModelInstance(vojtaBallHandle);
        vojtaBall.setPosition(glm::vec3(0.f, 0.f, 0.f));
        current_scene->add_model("vojta_ball", std::move(vojtaBall));
    }
    auto boxHandle =
        createSimpleModel(assets, "box_model", assets.getHandle<Mesh>("cube"),
            assets.getHandle<Texture>("wood_box"),
            assets.getHandle<ShaderProgram>("phong"));
    {
        ModelInstance m_box_texture = createModelInstance(boxHandle);
        m_box_texture.setPosition(glm::vec3(2.f, 0.f, 0.f));
        current_scene->add_static_model("m_box_texture", std::move(m_box_texture),
            assets);
    }

    {
        ModelInstance glass = createModelInstance(boxHandle);
        glass.setPosition(glm::vec3(2.f, 0.5f, 2.f));
        glass.setScale(glm::vec3(2.0f, 2.0f, 0.05f));
        glass.color_override = glm::vec4(0.3f, 0.6f, 1.0f, 1.0f);
        glass.is_transparent = true;
        glass.opacity = 0.3f;
        current_scene->add_static_model("glass_pane", std::move(glass), assets);
    }

    auto bunnyHandle =
        createSimpleModel(assets, "bunny_model", assets.getHandle<Mesh>("bunny"),
            assets.getHandle<Texture>("TextureDouble_A"),
            assets.getHandle<ShaderProgram>("phong"));
    {
        ModelInstance bunny = createModelInstance(bunnyHandle);
        bunny.setPosition(glm::vec3(4.2f, -0.5f, 0.f));
        bunny.setEulerAngles(glm::vec3(0.f, 335.f, 0.f));
        bunny.setScale(glm::vec3(0.8f, 0.8f, 0.8f));
        bunny.opacity = 0.5f;
        current_scene->add_static_model("bunny", std::move(bunny), assets);
    }

    auto dragonHandle = createSimpleModel(
        assets, "dragon_model", assets.getHandle<Mesh>("dragon"),
        assets.getHandle<Texture>("default"),
        assets.getHandle<ShaderProgram>("phong"));
    {
        ModelInstance dragon = createModelInstance(dragonHandle);
        dragon.setPosition(glm::vec3(6.f, 0.f, 0.f));
        dragon.setScale(glm::vec3(1.8f, 1.8f, 1.8f));
        // dragon.enabled = false;
        current_scene->add_static_model("dragon", std::move(dragon), assets);
    }

    auto wallHandle = createSimpleModel(
        assets, "wall_model", assets.getHandle<Mesh>("wall"),
        assets.getHandle<Texture>("wall_tex"),
        assets.getHandle<ShaderProgram>("phong"));

    // === SHOOTING RANGE LAYOUT ===

    // 1. 20 Static Walls (forming L-corners near edges, some L-corners near center, and some straight blockers, y = -1.0f)
    struct WallSpawn {
        glm::vec3 pos;
        float yaw;
    };
    WallSpawn walls[20] = {
        // L-Corner 1 (Outer Bottom Left)
        { {-25.f, -1.0f, -12.f}, 0.f },
        { {-24.f, -1.0f, -11.f}, 90.f },

        // L-Corner 2 (Inner Bottom Left)
        { {-10.f, -1.0f, -6.f}, 0.f },
        { {-9.f, -1.0f, -5.f}, 90.f },

        // L-Corner 3 (Outer Top Left)
        { {-25.f, -1.0f, 12.f}, 0.f },
        { {-24.f, -1.0f, 11.f}, 90.f },

        // L-Corner 4 (Inner Top Right)
        { {10.f, -1.0f, 6.f}, 0.f },
        { {9.f, -1.0f, 5.f}, 90.f },

        // L-Corner 5 (Outer Bottom Right)
        { {25.f, -1.0f, -12.f}, 0.f },
        { {24.f, -1.0f, -11.f}, 90.f },

        // L-Corner 6 (Outer Top Right)
        { {25.f, -1.0f, 12.f}, 0.f },
        { {24.f, -1.0f, 11.f}, 90.f },

        // Straight walls inside the scene (widely spaced)
        { {-18.f, -1.0f, -2.f}, 90.f },
        { {-15.f, -1.0f, 4.f}, 0.f },
        { {-5.f, -1.0f, 10.f}, 90.f },
        { {0.f, -1.0f, -8.f}, 0.f },
        { {5.f, -1.0f, -2.f}, 90.f },
        { {15.f, -1.0f, -9.f}, 0.f },
        { {18.f, -1.0f, 2.f}, 90.f },
        { {0.f, -1.0f, 14.f}, 90.f }
    };
    for (int i = 0; i < 20; ++i) {
        ModelInstance wall = createModelInstance(wallHandle);
        wall.setPosition(walls[i].pos);
        wall.setEulerAngles(glm::vec3(0.f, walls[i].yaw, 0.f));
        wall.setScale(glm::vec3(1.f, 1.f, 1.f));
        current_scene->add_static_model("wall_brick_" + std::to_string(i), std::move(wall), assets);
    }

    // 2. 10 Glass Walls (y = -1.0f, same wall model, but transparent & ricochet)
    WallSpawn glass_panes[10] = {
        { {-20.f, -1.0f, -8.f}, 90.f },
        { {-18.f, -1.0f, 8.f}, 0.f },
        { {-12.f, -1.0f, 0.f}, 90.f },
        { {-5.f, -1.0f, -12.f}, 0.f },
        { {0.f, -1.0f, 10.f}, 90.f },
        { {5.f, -1.0f, -10.f}, 0.f },
        { {12.f, -1.0f, 0.f}, 90.f },
        { {18.f, -1.0f, -8.f}, 0.f },
        { {20.f, -1.0f, 8.f}, 90.f },
        { {0.f, -1.0f, -2.f}, 0.f }
    };
    for (int i = 0; i < 10; ++i) {
        ModelInstance glass = createModelInstance(wallHandle);
        glass.setPosition(glass_panes[i].pos);
        glass.setEulerAngles(glm::vec3(0.f, glass_panes[i].yaw, 0.f));
        glass.setScale(glm::vec3(1.f, 1.f, 1.f));
        glass.color_override = glm::vec4(0.3f, 0.6f, 1.0f, 1.0f);
        glass.is_transparent = true;
        glass.opacity = 0.3f;
        current_scene->add_static_model("glass_" + std::to_string(i), std::move(glass), assets);
    }

    // 3. 30 Boxes in designated configurations + Targets
    int box_counter = 0;
    int teapot_counter = 0;
    int dragon_counter_idx = 0;
    int bunny_counter = 0;

    // Helper to spawn a box
    auto spawnBox = [&](const glm::vec3& pos, float rotation) {
        std::string name = "box_range_" + std::to_string(box_counter++);
        ModelInstance box = createModelInstance(boxHandle);
        box.setPosition(pos);
        box.setEulerAngles(glm::vec3(0.f, rotation, 0.f));
        box.setScale(glm::vec3(1.f, 1.f, 1.f));
        current_scene->add_static_model(name, std::move(box), assets);
        };

    // Helper to spawn a Teapot Target (on a box)
    auto spawnTeapotTarget = [&](const glm::vec3& pos) {
        std::string name = "teapot_target_" + std::to_string(teapot_counter++);
        ModelInstance teapot = createModelInstance(teapotHandle);
        teapot.setPosition(pos);
        teapot.setScale(glm::vec3(0.1f, 0.1f, 0.1f));
        teapot.opacity = 1.0f;
        current_scene->add_static_model(name, std::move(teapot), assets);
        };

    // Helper to spawn a Dragon Target (top of 2 stacked boxes)
    auto spawnDragonTarget = [&](const glm::vec3& pos) {
        std::string name = "dragon_target_" + std::to_string(dragon_counter_idx++);
        ModelInstance dragon = createModelInstance(dragonHandle);
        dragon.setPosition(pos + glm::vec3(0.f, 0.5f, 0.f));
        dragon.setScale(glm::vec3(1.8f, 1.8f, 1.8f));
        current_scene->add_static_model(name, std::move(dragon), assets);
        };

    // Helper to spawn a Bunny Target (on the ground)
    auto spawnBunnyTarget = [&](const glm::vec3& pos) {
        std::string name = "bunny_target_" + std::to_string(bunny_counter++);
        ModelInstance bunny = createModelInstance(bunnyHandle);
        bunny.setPosition(pos);
        bunny.setScale(glm::vec3(0.8f, 0.8f, 0.8f));
        bunny.opacity = 1.0f;
        current_scene->add_static_model(name, std::move(bunny), assets);
        };

    // Config A: 6 Standalone boxes (Y = -0.5f) -> Teapots on ONLY SOME of them (Y = 0.0f)
    glm::vec3 standalone_positions[6] = {
        {-28.f, -0.5f, -13.f},
        {-14.f, -0.5f, 10.f},
        {-6.f, -0.5f, -4.f},
        {6.f, -0.5f, -9.f},
        {14.f, -0.5f, 3.f},
        {28.f, -0.5f, 13.f}
    };
    float standalone_rotations[6] = { 15.f, 45.f, 70.f, -30.f, 110.f, -15.f };
    for (int i = 0; i < 6; ++i) {
        spawnBox(standalone_positions[i], standalone_rotations[i]);
        // Spawn teapot on only 3 of the standalone boxes (e.g. indices 0, 2, 4)
        if (i % 2 == 0) {
            spawnTeapotTarget(standalone_positions[i] + glm::vec3(0.f, 0.5f, 0.f));
        }
    }

    // Config B: 6 Stacked box pairs (12 boxes total) -> Dragons on top box of first 2 pairs, Teapots on some of the rest
    glm::vec3 stacked_positions[6] = {
        {-22.f, 0.0f, -4.f},  // Pair 0 -> Dragon 1
        {-9.f, 0.0f, 9.f},    // Pair 1 -> Dragon 2
        {-3.f, 0.0f, -13.f},  // Pair 2 -> Empty
        {8.f, 0.0f, 13.f},    // Pair 3 -> Teapot
        {20.f, 0.0f, -9.f},   // Pair 4 -> Empty
        {26.f, 0.0f, -2.f}    // Pair 5 -> Teapot
    };
    float stacked_rotations[6] = { 12.f, -40.f, 5.f, 25.f, -80.f, 45.f };
    for (int i = 0; i < 6; ++i) {
        // Bottom box (Y = -0.5)
        spawnBox(stacked_positions[i] + glm::vec3(0.f, -0.5f, 0.f), stacked_rotations[i]);
        // Top box (Y = 0.5)
        spawnBox(stacked_positions[i] + glm::vec3(0.f, 0.5f, 0.f), stacked_rotations[i] + 15.f);

        // Spawn targets: Only max 2 dragons, and teapots only on some (Pair 3 and 5)
        if (i < 2) {
            spawnDragonTarget(stacked_positions[i] + glm::vec3(0.f, 1.0f, 0.f));
        } else if (i == 3 || i == 5) {
            spawnTeapotTarget(stacked_positions[i] + glm::vec3(0.f, 1.0f, 0.f));
        }
    }

    // Config C: 3 groups of "2-stacked + 2 adjacent" (12 boxes total)
    // Adjacent on ground: Y = -0.5f
    struct GroupC {
        glm::vec3 center; // Center of the stacked pair
        float rot;
        glm::vec3 adj1_offset;
        glm::vec3 adj2_offset;
    };
    GroupC groups[3] = {
        { {-16.f, 0.0f, -9.f}, 0.f, {-1.0f, 0.f, 0.f}, {0.f, 0.f, -1.0f} },
        { {-1.f, 0.0f, 6.f}, 30.f, {0.f, 0.f, -1.0f}, {1.0f, 0.f, 0.f} },
        { {16.f, 0.0f, -13.f}, -15.f, {-1.0f, 0.f, 0.f}, {0.f, 0.f, 1.0f} }
    };
    for (int i = 0; i < 3; ++i) {
        glm::vec3 c = groups[i].center;
        float r = groups[i].rot;
        // Stacked bottom (Y = -0.5)
        spawnBox(c + glm::vec3(0.f, -0.5f, 0.f), r);
        // Stacked top (Y = 0.5)
        spawnBox(c + glm::vec3(0.f, 0.5f, 0.f), r + 20.f);

        // Spawn teapot target only on the top box of group 0
        if (i == 0) {
            spawnTeapotTarget(c + glm::vec3(0.f, 1.0f, 0.f));
        }

        // Adjacent 1 on ground (Y = -0.5)
        spawnBox(c + glm::vec3(groups[i].adj1_offset.x, -0.5f, groups[i].adj1_offset.z), r - 10.f);
        // Spawn teapot target only on adjacent 1 of group 0 and group 1
        if (i == 0 || i == 1) {
            spawnTeapotTarget(c + glm::vec3(groups[i].adj1_offset.x, 0.0f, groups[i].adj1_offset.z));
        }

        // Adjacent 2 on ground (Y = -0.5)
        spawnBox(c + glm::vec3(groups[i].adj2_offset.x, -0.5f, groups[i].adj2_offset.z), r + 25.f);
        // Spawn teapot target only on adjacent 2 of group 2
        if (i == 2) {
            spawnTeapotTarget(c + glm::vec3(groups[i].adj2_offset.x, 0.0f, groups[i].adj2_offset.z));
        }
    }

    // 4. Max 5 Bunnies on the ground (Y = -1.0f)
    glm::vec3 bunny_positions[5] = {
        {-27.f, -1.0f, -5.f},
        {-12.f, -1.0f, 13.f},
        {0.f, -1.0f, -7.f},
        {15.f, -1.0f, 8.f},
        {27.f, -1.0f, -3.f}
    };
    for (int i = 0; i < 5; ++i) {
        spawnBunnyTarget(bunny_positions[i]);
    }

    auto sponzaHandle = createSimpleModel(assets, "sponza_model", assets.getHandle<Mesh>("sponza"), assets.getHandle<Texture>("default"), assets.getHandle<ShaderProgram>("phong"));
    {
        ModelInstance sponza = createModelInstance(sponzaHandle);
        sponza.setScale(glm::vec3(0.02f, 0.02f, 0.02f));
        sponza.setPosition(glm::vec3(0.0f, -1.f, 0.0f));
        current_scene->add_static_model("sponza", std::move(sponza), assets, false);
    }

    auto planeHandle = createSimpleModel(assets, "plane_model", assets.getHandle<Mesh>("plane"), assets.getHandle<Texture>("default"), assets.getHandle<ShaderProgram>("phong"));
    {
        ModelInstance floor_plane = createModelInstance(planeHandle);
        floor_plane.setPosition(glm::vec3(0.f, -1.0f, 0.f));
        floor_plane.setScale(glm::vec3(50.f, 1.f, 50.f));
        floor_plane.setEulerAngles(glm::vec3(180.f, 0.f, 0.f));
        current_scene->add_static_model("floor_plane", std::move(floor_plane),
            assets);
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

    auto skybox = std::make_shared<Skybox>(assets.getHandle<Mesh>("skybox_triangle"), cm, assets.getHandle<ShaderProgram>("skybox"));
    skyboxes.push_back(skybox);
    // current_scene->skybox = skybox;

    main_camera = std::make_shared<Camera>();
    main_camera->Position = glm::vec3(-10.0f, 6.0f, 1.2f);
    main_camera->Yaw = 90.f;
    main_camera->ProcessMouseMovement(0, 0);
    current_scene->camera = main_camera;

    axis_display.init(&assets);
    axis_display.set_viewport(0, 0, 64, 64);

    scenes.push_back(current_scene);
}

// MARK: RUN
int App::run() {
    try {

        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        glfwGetCursorPos(window, &last_cursor_pos_x, &last_cursor_pos_y);

        glViewport(0, 0, fb_width, fb_height);
        update_projection_matrix();

        current_scene->set_light(0, glm::vec4(-0.75f, -1.f, -0.333f, 0.f), glm::vec4(0.f), glm::vec4(0.45f, 0.45f, 0.45f, 1.f), 1.f, 0.f); // sun
        current_scene->set_light(1, glm::vec4(-1.f, 2.f, -2.f, 1.f), glm::vec4(0.f), glm::vec4(1.f, 0.f, 0.f, 1.f), 0.15f, 180.f);
        current_scene->set_light(2, glm::vec4(-3.5f, 2.6f, 0.f, 1.f), glm::vec4(0.f), glm::vec4(0.f, 1.f, 0.f, 1.f), 0.10f, 180.f);
        current_scene->set_light(3, glm::vec4(0.f, 2.f, 3.f, 1.f), glm::vec4(0.f), glm::vec4(0.f, 0.f, 1.f, 1.f), 0.07f, 180.f);

        // flashlight
        current_scene->set_light(4, glm::vec4(0.f), glm::vec4(0.f), glm::vec4(1.f, 0.95f, 0.8f, 1.f), 0.05f, 15.f);
        current_scene->active_lights = 5;

        auto phongShaderHandle = assets.getHandle<ShaderProgram>("phong");
        auto chaosShaderHandle = assets.getHandle<ShaderProgram>("chaos");
        float chaosOff = 0.01f;
        float chaosExp = -1.9f;

        float rotation_speed = 0.f;
        int debugMode = 0;

        glClearColor(0.1f, 0.1f, 0.1f, 1);
        double last_time = -1 / 60.0;
        double last_fps_time = 0.0;

        // Uložení výchozích pozic (přidat před while smyčku)
        std::vector<glm::vec4> initial_light_positions;
        for (int i = 0; i < current_scene->active_lights; i++) {
            initial_light_positions.push_back(current_scene->lights.position[i]);
        }

        while (!glfwWindowShouldClose(window)) {

            imgui->new_frame();
            imgui->gui_begin();

            if (app_settings.gui_enabled || app_settings.gui_always_enabled) {

                if (app_settings.gui_always_enabled && !app_settings.gui_enabled) {
                    ImGui::BeginDisabled();
                }

                ImGui::Text("FPS: %4.1f %4.0f %4.0f ", FPS.get_current(),
                    FPS.get_1_low(), FPS.get_01_low());
                if (ImGui::Checkbox("VSync", &this->app_settings.vsync)) {
                    set_vsync(this->app_settings.vsync);
                }
                if (ImGui::Checkbox("Antialiasing (MSAA 4x)",
                    &this->app_settings.msaa_enabled)) {
                    set_msaa(this->app_settings.msaa_enabled);
                }
                if (ImGui::Checkbox("Fullscreen", &this->app_settings.fullscreen)) {
                    set_fullscreen(app_settings.fullscreen);
                }
                ImGui::Checkbox("Axis display",
                    &this->app_settings.gui_axis_display_enabled);
                ImGui::Checkbox("GUI always active",
                    &this->app_settings.gui_always_enabled);

                ImGui::Separator();
                ImGui::Checkbox("Debug Draw AABBs",
                    &this->app_settings.debug_draw_aabb);
                ImGui::Checkbox("Debug Freeze Frustum",
                    &this->app_settings.debug_freeze_frustum);

                ImGui::SliderFloat("Rotation speed", &rotation_speed, 0.f, 10.f);
                if (ImGui::SliderFloat("FoV", &current_scene->camera->fov, 20.f,
                    180.f)) {
                    update_projection_matrix();
                }
                ImGui::SliderFloat("Camera speed",
                    &current_scene->camera->MovementSpeed, 0.f, 10.f);
                ImGui::SliderFloat("Chaos Offset", &chaosOff, 0.f, 2.f);
                ImGui::SliderFloat("Chaos Exp", &chaosExp, -2.f, 2.f);

                ImGui::Checkbox("Demo Window", &imgui->debug_window_open);
                ImGui::Checkbox("Camera Collisions (C)", &camera_collisions_enabled);

                ImGui::Combo("Shader debug", &debugMode, ShaderDebugOptions,
                    sizeof(ShaderDebugOptions) / sizeof(const char*));

                ImGui::Text("Camera:");
                ImGui::Text("x: %.2f | y: %.2f | z: %.2f", main_camera->Position.x,
                    main_camera->Position.y, main_camera->Position.z);
                ImGui::Text("pitch: %.1f | yaw: %.1f", main_camera->Pitch,
                    main_camera->Yaw);

                ImGui::Text("meshes: %d", renderer.mesh_count);

                if (ImGui::TreeNode("current_scene")) {
                    size_t i = 0;
                    for (auto& [name, model] : current_scene->get_all_models()) {
                        {
                            ImGui::PushID(i);
                            if (ImGui::TreeNode("", name.c_str())) {
                                bool is_enabled = current_scene->is_model_enabled(name);
                                bool was_enabled = is_enabled;
                                bool is_static = current_scene->is_model_static(name);
                                AppImGui::model_controls(&model, &is_enabled, is_static);
                                if (is_enabled != was_enabled) {
                                    current_scene->set_model_enabled(name, is_enabled);
                                }
                                ImGui::TreePop();
                            }
                            ImGui::PopID();
                            i++;
                        }
                    }
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Lights")) {
                    ImGui::SliderInt("Active", &current_scene->active_lights, 0, 16);
                    for (size_t i = 0; i < MAX_LIGHTS; i++) {
                        ImGui::PushID(i);
                        if (ImGui::TreeNode("", "light[%d]", i)) {
                            AppImGui::light_controls(&current_scene->lights, i);
                            ImGui::TreePop();
                        }
                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Skybox")) {
                    static int current_skybox_index = 0;
                    if (ImGui::SliderInt("Skybox", &current_skybox_index, 0,
                        skyboxes.size())) {
                        if (current_skybox_index - 1 >= 0 &&
                            current_skybox_index - 1 < skyboxes.size())
                            current_scene->skybox = skyboxes.at(current_skybox_index - 1);
                        else
                            current_scene->skybox = nullptr;
                    }

                    ImGui::TreePop();
                }

                if (app_settings.gui_always_enabled && !app_settings.gui_enabled) {
                    ImGui::EndDisabled();
                }
            }

            // Scoreboard
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 150.f, 24.f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(126.f, 0.f));
            ImGui::Begin("Scoreboard", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
            ImGui::Text("SCORE: %d", player_score);
            ImGui::End();

            imgui->gui_end();

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            auto now = glfwGetTime();
            double delta_time = now - last_time;
            last_time = now;

            glm::vec3 velocity = main_camera->ProcessInput(window, delta_time);

            auto get_camera_aabb = [](glm::vec3 pos) {
                // half sizes: width 1m (x=0.5), height 2m (y=1.0), depth 0.5m (z=0.25)
                glm::vec3 half_extents(0.5f, 1.0f, 0.25f);

                // hitbox "postavy," kamera ve výšce očí
                glm::vec3 center = pos - glm::vec3(0.0f, 0.8f, 0.0f);
                return AABB{ center - half_extents, center + half_extents };
                };

            if (glm::length(velocity) > 0.0f) {
                // X axis
                main_camera->Position.x += velocity.x;
                if (camera_collisions_enabled && current_scene->check_collision(
                    get_camera_aabb(main_camera->Position), assets)) {
                    main_camera->Position.x -= velocity.x;
                }

                // Y axis
                main_camera->Position.y += velocity.y;
                if (camera_collisions_enabled && current_scene->check_collision(
                    get_camera_aabb(main_camera->Position), assets)) {
                    main_camera->Position.y -= velocity.y;
                }

                // Z axis
                main_camera->Position.z += velocity.z;
                if (camera_collisions_enabled && current_scene->check_collision(
                    get_camera_aabb(main_camera->Position), assets)) {
                    main_camera->Position.z -= velocity.z;
                }
            }

            // MARK: Shooting logic
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !imgui->capture_mouse()) {
                if (now - last_shot_time > 0.2) {
                    last_shot_time = now;
                    std::string p_name = "blaster_shot_" + std::to_string(projectile_counter++);

                    ModelInstance shot = createModelInstance(assets.getHandle<ModelResource>("box_model"));
                    shot.pivot_position = main_camera->Position + main_camera->Front * 0.5f;

                    float pitch_x = glm::degrees(atan2(-main_camera->Front.y, main_camera->Front.z));
                    float yaw_y = glm::degrees(asin(glm::clamp(main_camera->Front.x, -1.0f, 1.0f)));
                    shot.eulerAngles = glm::vec3(pitch_x, yaw_y, 0.0f);
                    shot.scale = glm::vec3(0.05f, 0.05f, 0.5f);
                    shot.color_override = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
                    shot.collision_enabled = false; // The projectile itself shouldn't act as a static wall

                    current_scene->add_model(p_name, std::move(shot));

                    Projectile p;
                    p.name = p_name;
                    p.position = shot.pivot_position;
                    p.direction = main_camera->Front;
                    p.lifetime = 2.0f; // 2 seconds
                    projectiles.push_back(p);
                }
            }

            // Update projectiles
            for (auto it = projectiles.begin(); it != projectiles.end(); ) {
                it->lifetime -= (float)delta_time;
                if (it->lifetime <= 0.0f) {
                    current_scene->remove_model(it->name);
                    it = projectiles.erase(it);
                    continue;
                }

                float speed = 20.0f;
                it->position += it->direction * speed * (float)delta_time;

                // collision check
                AABB p_aabb;
                p_aabb.min = it->position - glm::vec3(0.05f);
                p_aabb.max = it->position + glm::vec3(0.05f);

                std::string hit_model = current_scene->get_collided_model_name(p_aabb, assets);
                if (!hit_model.empty()) {
                    if (hit_model.find("teapot") != std::string::npos) {
                        player_score += 1;
                        current_scene->set_model_enabled(hit_model, false);
                        respawn_queue.push_back({ hit_model, now + 5.0 });
                    } else if (hit_model.find("bunny") != std::string::npos) {
                        player_score += 5;
                        current_scene->set_model_enabled(hit_model, false);
                        respawn_queue.push_back({ hit_model, now + 5.0 });
                    } else if (hit_model.find("dragon") != std::string::npos) {
                        player_score += 10;
                        current_scene->set_model_enabled(hit_model, false);
                        respawn_queue.push_back({ hit_model, now + 5.0 });
                    } else if (hit_model.find("glass") != std::string::npos) {
                        // Ricochet logic
                        auto aabbs = current_scene->get_model_aabbs(hit_model, assets);
                        if (!aabbs.empty()) {
                            AABB glass_aabb = aabbs[0];
                            glm::vec3 center = (glass_aabb.min + glass_aabb.max) * 0.5f;
                            glm::vec3 extents = (glass_aabb.max - glass_aabb.min) * 0.5f;
                            glm::vec3 d = it->position - center;

                            glm::vec3 ratio = glm::abs(d) / extents;
                            glm::vec3 normal;
                            if (ratio.x > ratio.y && ratio.x > ratio.z) {
                                normal = glm::vec3(glm::sign(d.x), 0, 0);
                            } else if (ratio.y > ratio.x && ratio.y > ratio.z) {
                                normal = glm::vec3(0, glm::sign(d.y), 0);
                            } else {
                                normal = glm::vec3(0, 0, glm::sign(d.z));
                            }

                            it->direction = glm::reflect(it->direction, normal);
                            it->position += normal * 0.2f; // Push out to avoid getting stuck

                            float pitch_x = glm::degrees(atan2(-it->direction.y, it->direction.z));
                            float yaw_y = glm::degrees(asin(glm::clamp(it->direction.x, -1.0f, 1.0f)));
                            current_scene->get_all_models().at(it->name).eulerAngles = glm::vec3(pitch_x, yaw_y, 0.0f);
                        }
                        continue;
                    }

                    current_scene->remove_model(it->name);
                    it = projectiles.erase(it);
                    continue;
                }

                // update scene model
                current_scene->get_all_models().at(it->name).pivot_position = it->position;

                ++it;
            }

            // Process respawn queue
            for (auto it = respawn_queue.begin(); it != respawn_queue.end(); ) {
                if (now >= it->respawn_time) {
                    current_scene->set_model_enabled(it->model_name, true);
                    it = respawn_queue.erase(it);
                } else {
                    ++it;
                }
            }


            // Rotating lights
            float light_rotation_speed = 0.5f;

            // Vytvoření matice rotace podle času (okolo osy Y: 0, 1, 0)
            glm::mat4 light_rot_matrix =
                glm::rotate(glm::mat4(1.0f), (float)now * light_rotation_speed,
                    glm::vec3(0.0f, 1.0f, 0.0f));

            // Přeuložení pozic aplikováním rotační matice
            for (size_t i = 0; i < std::min(current_scene->active_lights,
                (int)initial_light_positions.size());
                i++) {
                if (i == 4) continue; // flashlight is not rotated this way
                current_scene->lights.position[i] =
                    light_rot_matrix * initial_light_positions[i];
            }

            // Update flashlight (index 4)
            if (flashlight_enabled) {
                current_scene->lights.position[4] = glm::vec4(main_camera->Position, 1.0f);
                current_scene->lights.direction[4] = glm::vec4(main_camera->Front, 0.0f);
                current_scene->lights.color[4] = glm::vec4(1.f, 0.95f, 0.8f, 1.f);
            } else {
                current_scene->lights.color[4] = glm::vec4(0.f); // turn off
            }

            auto shader_phong = assets.getResourceMaybe(phongShaderHandle);
            if (shader_phong) {
                current_scene->update_shader_lights(shader_phong);
                shader_phong->setUniform("debugMode", debugMode);
            }

            auto chaos_shader = assets.getResourceMaybe(chaosShaderHandle);
            if (chaos_shader) {
                current_scene->update_shader_lights(chaos_shader);
                chaos_shader->setUniform("uTime", (float)now);
                chaos_shader->setUniform("uChaosOffset", chaosOff);
                chaos_shader->setUniform("uChaosExp", chaosExp);
            }

            for (auto&& [name, model_ptr] : current_scene->get_models()) {
                if (!current_scene->is_model_static(name)) {
                    model_ptr->rotate(glm::vec3(17.f * rotation_speed * delta_time,
                        31.f * rotation_speed * delta_time,
                        11.f * rotation_speed * delta_time));
                }
                model_ptr->update(delta_time);
            }

            auto proj = main_camera->GetProjMatrix();
            auto view = main_camera->GetViewMatrix();
            auto vp = proj * view;

            static glm::mat4 cached_vp = vp;
            static Frustum cached_frustum = Frustum::extractFrustum(vp);

            if (!app_settings.debug_freeze_frustum) {
                cached_vp = vp;
                cached_frustum = Frustum::extractFrustum(vp);
            }

            assets.update();
            renderer.debug_aabb = app_settings.debug_draw_aabb;
            renderer.debug_frustum = app_settings.debug_freeze_frustum;
            renderer.render(&assets, current_scene.get(), cached_frustum, cached_vp);

            if (app_settings.gui_axis_display_enabled)
                axis_display.draw(main_camera);

            imgui->render();

            glfwSwapBuffers(window);

            if (screenshot != 0) {
                static int saved_msaa = -1;

                std::string ss = std::format("screenshots/{:%Y-%m-%d_%H-%M-%S}",
                    std::chrono::system_clock::now());
                ss +=
                    (app_settings.msaa_enabled == 1 ? "_msaa-on.png" : "_msaa-off.png");
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
                std::cout << std::format("FPS: {:6.1f} | 1%: {:3.0f} | 0.1%: {:3.0f}",
                    FPS.get_current(), FPS.get_1_low(),
                    FPS.get_01_low())
                    << std::endl;
            }
        }
    }
    catch (std::exception const& e) {
        std::cerr << "App failed : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
// MARK: END RUN

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

        if (!app_settings.window_maximized) {
            saved_window_pos_x = app_settings.window_pos_x;
            saved_window_pos_y = app_settings.window_pos_y;
            saved_window_width = app_settings.window_width;
            saved_window_height = app_settings.window_height;
        }

        saved_refresh_rate = mode->refreshRate;

        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height,
            mode->refreshRate);
    } else {
        glfwSetWindowMonitor(window, NULL, saved_window_pos_x, saved_window_pos_y,
            saved_window_width, saved_window_height, NULL);
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
    glReadPixels(0, 0, fb_width, fb_height, GL_RGBA, GL_UNSIGNED_BYTE,
        framebuffer.data());

    std::filesystem::path filePath(path);

    auto dir = filePath.parent_path();
    if (!dir.empty()) {
        std::filesystem::create_directories(dir);
    }

    if (imwrite(filePath.string(), framebuffer, 1) == 0) {
        std::cout << "Failed to save screenshot! (path:\"" << path << "\")"
            << std::endl;
    }
}

void App::update_projection_matrix(void) {
    if (fb_height < 1)
        fb_height = 1; // avoid division by 0

    float ratio = static_cast<float>(fb_width) / fb_height;

    main_camera->update_projection_matrix(ratio);

    axis_display.viewport[2] = (GLsizei)std::floorf(fb_width / 15.f);
    axis_display.viewport[3] = (GLsizei)std::floorf(fb_width / 15.f);
}

App::~App() {
    settings::save("settings.json", app_settings);

    delete imgui;
}
