#pragma once
#include <unordered_map>

#include "ModelInstance.hpp"

struct Scene
{
    std::unordered_map<std::string, ModelInstance> models;
    //DirectionalLight sun;
};
