#pragma once
#ifndef _glfw3_h_
#include <GLFW/glfw3.h>
#endif

template <auto Method>
struct GlfwBinder
{
    template <typename... Args>
    static void callback(GLFWwindow* window, Args... args)
    {
        if (auto* app = static_cast<App*>(glfwGetWindowUserPointer(window)))
        {
            (app->*Method)(args...);
        }
    }
};

#define GLEW_CHECK(err, name) \
do {\
    if (err != GLEW_OK) {\
        auto err_msg = (const char*)glewGetErrorString(err);\
        if (err_msg != nullptr) {\
            throw new std::runtime_error(std::format(name " failed!\nError: {}", err_msg).c_str());\
        } else {\
            throw new std::runtime_error(name " failed!");\
        }\
    }\
} while (0);\

#define GL_PRINT_STRING(name)\
[](){\
    const char * mystring = (const char*)glGetString(name);\
    if (mystring == nullptr)\
        std::cout << #name ": <Unknown>" << std::endl;\
    else\
        std::cout << #name": " << mystring << std::endl;\
    return mystring;\
}();\

#define GL_PRINT_NUMBER(name)\
[](){\
    int number = -1;\
    glGetIntegerv(name, &number);\
    std::cout << #name ": " << number << std::endl;\
    return number;\
}();\

#define GL_PRINT_FLAG(flags, flag) \
[flags](){\
    std::cout << #flag << ":" << ((flags & flag) ? "YES" : "NO") << std::endl;\
}();
