// icp.cpp 
// author: JJ

#include <iostream>
#include <opencv2/opencv.hpp>

#include "app.hpp"

//
// WARNING:
// In general, you can NOT freely reorder includes!
//

// C++
// include anywhere, in any order
#include <iostream>
#include <chrono>
#include <stack>
#include <random>
#include <format>

// OpenCV (does not depend on GL)
#include <opencv2\opencv.hpp>

// OpenGL Extension Wrangler: allow all multiplatform GL functions
#include <GL/glew.h> 
// WGLEW = Windows GL Extension Wrangler (change for different platform) 
// platform specific functions (in this case Windows)
#include <GL/wglew.h> 

// GLFW toolkit
// Uses GL calls to open GL context, i.e. GLEW __MUST__ be first.
#include <GLFW/glfw3.h>

// OpenGL math (and other additional GL libraries, at the end)
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "gl_err_callback.h"
#include "glfw_helpers.hpp"

//---------------------------------------------------------------------

App::App() {
    // default constructor
    // nothing to do here (so far...)
    std::cout << "Constructed...\n";
}

bool App::init() {

    if (!settings::load("settings.json", app_settings)) {
        std::cout << "Settings file settings.json doesnt exist" << std::endl;
    }

    // GL init
    {
        glfwSetErrorCallback(glfw_error_callback);
        // init glfw
        // https://www.glfw.org/documentation.html
        if (!glfwInit()) {
            throw new std::exception("glfwInit failed!");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        if (app_settings.window_pos_x != -1 || app_settings.window_pos_y != -1) {
            glfwWindowHint(GLFW_POSITION_X, app_settings.window_pos_x);
            glfwWindowHint(GLFW_POSITION_Y, app_settings.window_pos_y);
        }

        // open window (GL canvas) with no special properties
        // https://www.glfw.org/docs/latest/quick.html#quick_create_window
        window = glfwCreateWindow(app_settings.window_width, app_settings.window_height, "OpenGL context", NULL, NULL);
        if (!window) {
            glfwTerminate();
            throw new std::runtime_error("glfwCreateWindow failed!");
        }

        glfwMakeContextCurrent(window);
        glfwSetWindowUserPointer(window, this);

        // init glew
        // http://glew.sourceforge.net/basic.html
        GLenum err;
        err = glewInit();
        GLEW_CHECK(err, "glewInit");
        err = wglewInit();
        GLEW_CHECK(err, "wglewInit");

        if (!GLEW_ARB_direct_state_access)
            throw std::runtime_error("No DSA :-(");

        //Init debug
        if (GLEW_ARB_debug_output) {
            glDebugMessageCallback(MessageCallback, 0);
            glEnable(GL_DEBUG_OUTPUT);

            //default is asynchronous debug output, use this to simulate glGetError() functionality
            //glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

            std::cout << "GL_DEBUG enabled." << std::endl;
        } else {
            std::cout << "GL_DEBUG NOT SUPPORTED!" << std::endl;
        }

        // for current GL context set vsync
        glfwSwapInterval(app_settings.vsync);

        print_gl_info();
    }

    init_assets();

    init_callbacks();

    return 0;
}

void App::init_assets(void) {
    //
    // Initialize pipeline: compile, link and use shaders
    //

    //SHADERS - define & compile & link
    const char* vertex_shader =
        "#version 460 core\n"
        "in vec3 attribute_Position;"
        "void main() {"
        "  gl_Position = vec4(attribute_Position, 1.0);"
        "}";

    const char* fragment_shader =
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

    //now we can delete shader parts (they can be reused, if you have more shaders)
    //the final shader program already linked and stored separately
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
    //glVertexArrayAttribFormat(VAO_ID, position_attrib_location, vertex.position.length(), GL_FLOAT, GL_FALSE, offsetof(vertex, position));
    glVertexArrayAttribFormat(VAO_ID, position_attrib_location, some_vertex.position.length(), GL_FLOAT, GL_FALSE, offsetof(vertex, position));
    glVertexArrayAttribBinding(VAO_ID, position_attrib_location, 0); // (GLuint vaobj, GLuint attribindex, GLuint bindingindex)

    // Create and fill data
    glCreateBuffers(1, &VBO_ID);
    glNamedBufferData(VBO_ID, triangle_vertices.size() * sizeof(vertex), triangle_vertices.data(), GL_STATIC_DRAW);

    // Connect together
    glVertexArrayVertexBuffer(VAO_ID, 0, VBO_ID, 0, sizeof(vertex)); // (GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride)
}

int App::run() {
    GLfloat r, g, b, a;
    r = g = b = a = 1.0f; //white color

    // Activate shader program. There is only one program, so activation can be out of the loop. 
    // In more realistic scenarios, you will activate different shaders for different 3D objects.
    glUseProgram(shader_prog_ID);

    // Get uniform location in GPU program. This will not change, so it can be moved out of the game loop.
    GLint uniform_color_location = glGetUniformLocation(shader_prog_ID, "uniform_Color");
    if (uniform_color_location == -1) {
        std::cerr << "Uniform location is not found in active shader program. Did you forget to activate it?\n";
    }

    glClearColor(0, 0, 0, 1);

    auto start = std::chrono::steady_clock::now();
    auto end = start;
    std::chrono::duration<double> elapsed_seconds = end - start;
    double last_report_time = 0.0;
    double delta_time = 0.0;
    double last_time = 0.0;
    while (!glfwWindowShouldClose(window)) {
        double time_now = glfwGetTime();
        delta_time = time_now - last_time;
        last_time = time_now;

        start = std::chrono::steady_clock::now();

        // clear canvas
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        HSL data = HSL((int)(glfwGetTime() * (360 / 5)) % 360, 1.f, 0.5f);
        RGB value = HSLToRGB(data);

        //set uniform parameter for shader
        // (try to change the color in key callback)          
        glUniform4f(uniform_color_location, value.R / 255.f, value.G / 255.f, value.B / 255.f, a);

        //bind 3d object data
        glBindVertexArray(VAO_ID);

        // draw all VAO data
        glDrawArrays(GL_TRIANGLES, 0, triangle_vertices.size());

        // poll events, call callbacks, flip back<->front buffer
        glfwPollEvents();
        glfwSwapBuffers(window);

        end = std::chrono::steady_clock::now();
        elapsed_seconds = end - start;
        if (time_now - last_report_time > 0.05) {
            last_report_time = time_now;
            const auto fps = 1.0 / elapsed_seconds.count();
            const auto vsync_status = (app_settings.vsync ? "ON " : "OFF");
            glfwSetWindowTitle(window, std::format("OpenGL Window | VSync: {} | FPS: {:4.0f}", vsync_status, fps).c_str());
        }
    }

    return 0;
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
        //throw std::runtime_error("What??");
    }

    glGetIntegerv(GL_CONTEXT_FLAGS, &myint);
    GL_PRINT_FLAG(myint, GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT);
    GL_PRINT_FLAG(myint, GL_CONTEXT_FLAG_DEBUG_BIT);
    GL_PRINT_FLAG(myint, GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT);
    GL_PRINT_FLAG(myint, GL_CONTEXT_FLAG_NO_ERROR_BIT);
}

void App::init_callbacks() {
    glfwSetKeyCallback(window, GlfwBinder<&App::key_callback>::callback);
    glfwSetFramebufferSizeCallback(window, GlfwBinder<&App::fbsize_callback>::callback);
    glfwSetWindowPosCallback(window, GlfwBinder<&App::window_pos_callback>::callback);
    glfwSetMouseButtonCallback(window, GlfwBinder<&App::mouse_button_callback>::callback);
    glfwSetCursorPosCallback(window, GlfwBinder<&App::cursor_position_callback>::callback);
    glfwSetScrollCallback(window, GlfwBinder<&App::scroll_callback>::callback);
}

void App::error_callback(int error, const char* description) {
    std::cerr << "Error: " << description << std::endl;
}

void App::key_callback(int key, int scancode, int action, int mods) {
    if ((action == GLFW_PRESS) || (action == GLFW_REPEAT)) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_V:
            app_settings.vsync = !app_settings.vsync;
            glfwSwapInterval((app_settings.vsync ? 1 : 0));
            break;
        default:
            break;
        }
    }
}

void App::fbsize_callback(int width, int height) {
    std::cout << "fbsize_callback: width " << width << ", height " << height << std::endl;
    app_settings.window_width = width;
    app_settings.window_height = height;
}
void App::window_pos_callback(int xpos, int ypos) {
    \
        std::cout << "window_pos_callback: xpos " << xpos << ", ypos " << ypos << std::endl;
    app_settings.window_pos_x = xpos;
    app_settings.window_pos_y = ypos;
}
void App::mouse_button_callback(int button, int action, int mods) {
    std::cout << "mouse_button_callback: button " << button << ", action " << action << ", mods " << mods << std::endl;
}
void App::cursor_position_callback(double xpos, double ypos) {
    //std::cout << "cursor_position_callback: xpos " << xpos << ", ypos " << ypos << std::endl;
}
void App::scroll_callback(double xoffset, double yoffset) {
    if (yoffset > 0.0) {
        std::cout << "wheel up...\n";
    }
}

App::~App() {
    settings::save("settings.json", app_settings);

    //new stuff: cleanup GL data
    glDeleteProgram(shader_prog_ID);
    glDeleteBuffers(1, &VBO_ID);
    glDeleteVertexArrays(1, &VAO_ID);

}
