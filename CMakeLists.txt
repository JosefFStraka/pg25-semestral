cmake_minimum_required(VERSION 3.30)

project(icp VERSION 2025.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "MSVC_VERSION: ${MSVC_VERSION}")
#set(CMAKE_BUILD_TYPE Debug)

find_package(glm            CONFIG  REQUIRED)
find_package(GLEW                   REQUIRED)
find_package(glfw3          CONFIG  REQUIRED)
find_package(nlohmann_json  CONFIG  REQUIRED)
find_package(imgui          CONFIG  REQUIRED)

find_path(STB_INCLUDE_DIR "stb_image.h")
find_path(STB_INCLUDE_DIR "stb_image_write.h")

set(PROJECT_SOURCES
    app_imgui.cpp
    app.cpp
    callbacks.cpp
    engine/rendering/Texture.cpp
    engine/rendering/ShaderProgram.cpp
    gl_err_callback.cpp
    image_io.cpp
    main.cpp
    meshgen.cpp
    settings.cpp
    OBJloader.cpp
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE 
    GLEW::GLEW
    glfw
    glm::glm
    nlohmann_json::nlohmann_json
    imgui::imgui
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ImGui-GLFW-OpenGL3
    ${Stb_INCLUDE_DIR}
)