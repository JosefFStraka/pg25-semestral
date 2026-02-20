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
find_package(OpenCV         CONFIG  REQUIRED)
find_package(imgui          CONFIG  REQUIRED)

#find_package(Stb            REQUIRED)
#find_path(STB_INCLUDE_DIR "stb_image.h")

# if(WIN32)
#     find_package(OpenCV CONFIG REQUIRED PATHS "C:/Program Files/opencv/build")
# elseif(UNIX)
#     find_package(OpenCV CONFIG REQUIRED)
# endif()

set(PROJECT_SOURCES
    app_imgui.cpp
    app.cpp
    gl_err_callback.cpp
    main.cpp
    settings.cpp
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE 
    GLEW::GLEW
    glfw
    glm::glm
    nlohmann_json::nlohmann_json
    ${OpenCV_LIBS}
    imgui::imgui
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ImGui-GLFW-OpenGL3
#    ${Stb_INCLUDE_DIR}
)