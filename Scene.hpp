#pragma once
#include <unordered_map>

#include "ModelInstance.hpp"
#include "Camera.hpp"
#include "Skybox.hpp"

struct Scene
{
    std::unordered_map<std::string, ModelInstance> models;
    Camera* camera;
    Skybox* skybox;
    //DirectionalLight sun;
};
