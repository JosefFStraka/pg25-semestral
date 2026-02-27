#include "app.hpp"

#include <iostream>
#include <chrono>
#include <stack>
#include <random>
#include <format>

#include <opencv2\opencv.hpp>
#include <GL/glew.h>
#include <GL/wglew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "gl_err_callback.h"
#include "glfw_helpers.hpp"

#include "imgui.h"

//---------------------------------------------------------------------

App::App()
{
    // default constructor
    // nothing to do here (so far...)
    std::cout << "Constructed...\n";
}

bool App::init()
{
    try
    {
        std::cout << "Current working directory: " << std::filesystem::current_path().generic_string() << '\n';

        if (settings::load("settings.json", app_settings))
        {
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

        init_imgui();

        init_callbacks();

        // When all is loaded, show the window.
        glfwShowWindow(window);
    }
    catch (std::exception const &e)
    {
        std::cerr << "Init failed : " << e.what() << std::endl;
        throw;
    }

    return true;
}

void App::init_imgui(void)
{
    imgui = new AppImGui(window);
    imgui->init();
    std::cout << "ImGUI version: " << ImGui::GetVersion() << "\n";
}

void App::init_glfw(void)
{

    /* Initialize the library */
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit())
    {
        throw std::runtime_error("GLFW can not be initialized.");
    }

    // try to open OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // open window, but hidden - it will be enabled later, after asset initialization
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    if (app_settings.window_pos_x != -1 || app_settings.window_pos_y != -1)
    {
        glfwWindowHint(GLFW_POSITION_X, app_settings.window_pos_x);
        glfwWindowHint(GLFW_POSITION_Y, app_settings.window_pos_y);
    }

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(800, 600, "ICP", nullptr, nullptr);
    if (!window)
    {
        throw std::runtime_error("GLFW window can not be created.");
    }

    glfwSetWindowUserPointer(window, this);

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    // disable mouse cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLFW callbacks registration
    init_callbacks();
}

void App::init_glew()
{
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

void App::init_gl_debug(void)
{
    if (GLEW_ARB_debug_output)
    {
        glDebugMessageCallback(MessageCallback, 0);
        glEnable(GL_DEBUG_OUTPUT);

#ifndef DEBUG
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
#endif

        // default is asynchronous debug output, use this to simulate glGetError() functionality
        // glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

        std::cout << "GL_DEBUG enabled." << std::endl;
    }
    else
    {
        std::cout << "GL_DEBUG NOT SUPPORTED!" << std::endl;
    }
}

void App::init_assets(void)
{
    //
    // Initialize pipeline: compile, link and use shaders
    //

    // SHADERS - define & compile & link
    const char *vertex_shader =
        "#version 460 core\n"
        "in vec3 attribute_Position;"
        "void main() {"
        "  gl_Position = vec4(attribute_Position, 1.0);"
        "}";

    const char *fragment_shader =
        "#version 460 core\n"
        "uniform vec4 uniform_Color;"
        "out vec4 FragColor;"
        "void main() {"
        "  FragColor = uniform_Color;"
        "}";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    glCompileShader(fs);

    shader_prog_ID = glCreateProgram();
    glAttachShader(shader_prog_ID, fs);
    glAttachShader(shader_prog_ID, vs);
    glLinkProgram(shader_prog_ID);

    // now we can delete shader parts (they can be reused, if you have more shaders)
    // the final shader program already linked and stored separately
    glDetachShader(shader_prog_ID, fs);
    glDetachShader(shader_prog_ID, vs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    //
    // Create and load data into GPU using OpenGL DSA (Direct State Access)
    //

    // Create VAO + data description (similar to container)
    glCreateVertexArrays(1, &VAO_ID);

    GLint position_attrib_location = glGetAttribLocation(shader_prog_ID, "attribute_Position");

    vertex some_vertex;

    glEnableVertexArrayAttrib(VAO_ID, position_attrib_location);
    // glVertexArrayAttribFormat(VAO_ID, position_attrib_location, vertex.position.length(), GL_FLOAT, GL_FALSE, offsetof(vertex, position));
    glVertexArrayAttribFormat(VAO_ID, position_attrib_location, some_vertex.position.length(), GL_FLOAT, GL_FALSE, offsetof(vertex, position));
    glVertexArrayAttribBinding(VAO_ID, position_attrib_location, 0); // (GLuint vaobj, GLuint attribindex, GLuint bindingindex)

    // Create and fill data
    glCreateBuffers(1, &VBO_ID);
    glNamedBufferData(VBO_ID, triangle_vertices.size() * sizeof(vertex), triangle_vertices.data(), GL_STATIC_DRAW);

    // Connect together
    glVertexArrayVertexBuffer(VAO_ID, 0, VBO_ID, 0, sizeof(vertex)); // (GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride)
}

int App::run()
{

    try
    {
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

        // Activate shader program. There is only one program, so activation can be out of the loop.
        // In more realistic scenarios, you will activate different shaders for different 3D objects.
        glUseProgram(shader_prog_ID);

        // Get uniform location in GPU program. This will not change, so it can be moved out of the game loop.
        GLint uniform_color_location = glGetUniformLocation(shader_prog_ID, "uniform_Color");
        if (uniform_color_location == -1)
        {
            std::cerr << "Uniform location is not found in active shader program. Did you forget to activate it?\n";
        }

        glClearColor(0, 0, 0, 1);

        while (!glfwWindowShouldClose(window))
        {

            // ImGui prepare render (only if required)
            if (imgui->imgui_open)
            {
                imgui->new_frame();
                imgui->gui_begin();
                {
                    ImGui::Text("FPS: %.1f (%.1f - %.1f)", FPS.get_current(), FPS.get_min(), FPS.get_max());
                    if (ImGui::Checkbox("VSync", &this->app_settings.vsync))
                    {
                        glfwSwapInterval(this->app_settings.vsync);
                    }
                }
                imgui->gui_end();
            }

            //
            // UPDATE: recompute objects state, players position etc.
            //         nothing here so far...
            //

            //
            // RENDER: GL drawCalls
            //

            // Clear OpenGL canvas, both color buffer and Z-buffer
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // drawCalls to render whole scene
            // for (auto& [name, model] : scene) {
            // example: set uniform for specific model
            // if (name == "triangle") {
            //	model.shader.setUniform("uniform_Color", glm::vec4(glm::sin((float(glfwGetTime()))), g, b, a));
            //}

            //	model.draw();
            //}

            HSL data = HSL((int)(glfwGetTime() * (360 / 5)) % 360, 1.f, 0.5f);
            RGB value = HSLToRGB(data);
            glUniform4f(uniform_color_location, value.R / 255.f, value.G / 255.f, value.B / 255.f, 1.f);
            glBindVertexArray(VAO_ID);
            glDrawArrays(GL_TRIANGLES, 0, triangle_vertices.size());

            // ImGui display
            if (imgui->imgui_open)
            {
                imgui->render();
            }

            glfwSwapBuffers(window);

            glfwPollEvents();

            // FPS
            if (FPS.is_updated()) // display new value only once per interval (default = 1.0s)
                std::cout << "FPS: " << FPS.get_current() << " (min: " << FPS.get_min() << ", max: " << FPS.get_max() << ")" << std::endl;

            FPS.update();
        }
    }
    catch (std::exception const &e)
    {
        std::cerr << "App failed : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void App::print_gl_info()
{

    GL_PRINT_STRING(GL_VENDOR);
    GL_PRINT_STRING(GL_RENDERER);
    GL_PRINT_STRING(GL_VERSION);
    GL_PRINT_STRING(GL_SHADING_LANGUAGE_VERSION);

    GL_PRINT_NUMBER(GL_MAJOR_VERSION);
    GL_PRINT_NUMBER(GL_MINOR_VERSION);

    GLint myint;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &myint);

    if (myint & GL_CONTEXT_CORE_PROFILE_BIT)
    {
        std::cout << "We are using CORE profile\n";
    }
    else if (myint & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
    {
        std::cout << "We are using COMPATIBILITY profile\n";
    }
    else
    {
        // throw std::runtime_error("What??");
    }

    glGetIntegerv(GL_CONTEXT_FLAGS, &myint);
    GL_PRINT_FLAG(myint, GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT);
    GL_PRINT_FLAG(myint, GL_CONTEXT_FLAG_DEBUG_BIT);
    GL_PRINT_FLAG(myint, GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT);
    GL_PRINT_FLAG(myint, GL_CONTEXT_FLAG_NO_ERROR_BIT);
}

void App::error_callback(int error, const char *description)
{
    std::cerr << "Error: " << description << std::endl;
}

App::~App()
{
    settings::save("settings.json", app_settings);

    // new stuff: cleanup GL data
    glDeleteProgram(shader_prog_ID);
    glDeleteBuffers(1, &VBO_ID);
    glDeleteVertexArrays(1, &VAO_ID);
}
