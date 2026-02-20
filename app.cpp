// icp.cpp 
// author: JJ

#include <iostream>
#include <opencv2/opencv.hpp>

#include "app.hpp"

App::App() {
    // default constructor
    // nothing to do here (so far...)
    std::cout << "Constructed...\n";
}
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


#include "assets.hpp"
#include "gl_err_callback.h"

//---------------------------------------------------------------------

bool App::init() {

    // GL init
    {
        // init glfw
        // https://www.glfw.org/documentation.html
        // TODO: add error checking!
        glfwInit();


        // open window (GL canvas) with no special properties
        // https://www.glfw.org/docs/latest/quick.html#quick_create_window
        // TODO: add error checking!
        window = glfwCreateWindow(800, 600, "OpenGL context", NULL, NULL);
        glfwMakeContextCurrent(window);

        glfwSetWindowUserPointer(window, this);

        // init glew
        // http://glew.sourceforge.net/basic.html
        // TODO: add error checking!

        glewInit();
        wglewInit();

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

        // for current GL context switch vsync
        if (vsync)
            glfwSwapInterval(1);          // Set V-Sync ON.
        else
            glfwSwapInterval(0);          // Set V-Sync OFF.

        //TODO: get info about your GL context    
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

    auto start = std::chrono::steady_clock::now();
    auto end = std::chrono::steady_clock::now();
    auto last_print = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::chrono::duration<double> since_last_report;

    while (!glfwWindowShouldClose(window)) {
        start = std::chrono::steady_clock::now();

        // clear canvas
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //set uniform parameter for shader
        // (try to change the color in key callback)          
        glUniform4f(uniform_color_location, r, g, b, a);

        //bind 3d object data
        glBindVertexArray(VAO_ID);

        // draw all VAO data
        glDrawArrays(GL_TRIANGLES, 0, triangle_vertices.size());

        // poll events, call callbacks, flip back<->front buffer
        glfwPollEvents();
        glfwSwapBuffers(window);

        end = std::chrono::steady_clock::now();
        elapsed_seconds = end - start;
        since_last_report = end - last_print;
        if (since_last_report.count() > 1) {
            glfwSetWindowTitle(window, std::format("fps: {0:.0f}", 1.0 / elapsed_seconds.count()).c_str());
            last_print = end;
        }
    }

    return 0;
}


void App::init_callbacks() {
    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
            if (app)
                app->key_callback(key, scancode, action, mods);
        });
    //glfwSetFramebufferSizeCallback(window, fbsize_callback);
    //glfwSetMouseButtonCallback(window, mouse_button_callback);
    //glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset)
    {
        auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
        if (app)
            app->scroll_callback(xoffset, yoffset);
        });
}

void App::error_callback(int error, const char* description) {
    std::cerr << "Error: " << description << std::endl;
}

void App::scroll_callback(double xoffset, double yoffset) {
    if (yoffset > 0.0) {
        std::cout << "wheel up...\n";
    }
}

void App::key_callback(int key, int scancode, int action, int mods) {
    if ((action == GLFW_PRESS) || (action == GLFW_REPEAT)) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_V:
            // TODO: toggle VSync on-off by glfwSwapInterval(...)
            // hint: create bool variable in App class to remember last choice...
            vsync = !vsync;
            glfwSwapInterval((vsync ? 1 : 0));

            break;
        default:
            break;
        }
    }
}



App::~App() {
    //new stuff: cleanup GL data
    glDeleteProgram(shader_prog_ID);
    glDeleteBuffers(1, &VBO_ID);
    glDeleteVertexArrays(1, &VAO_ID);

}
