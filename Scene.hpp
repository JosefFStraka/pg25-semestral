#pragma once
#include <unordered_map>
#include <array>

#include "ModelInstance.hpp"
#include "Camera.hpp"
#include "Skybox.hpp"

// SoA 
constexpr int MAX_LIGHTS = 16;
struct s_lights {
    std::array<glm::vec4, MAX_LIGHTS> position;
    std::array<glm::vec4, MAX_LIGHTS> color;
    std::array<float, MAX_LIGHTS> attenuation;
    std::array<float, MAX_LIGHTS> spotCutoff;
};

struct Scene
{
    std::unordered_map<std::string, ModelInstance> models;
    std::shared_ptr<Camera> camera;
    std::shared_ptr<Skybox> skybox;
    s_lights lights;
    int active_lights{ 0 };

    void set_light(int i, glm::vec4 position, glm::vec4 color, float attenuation, float spotCutoff) {
        lights.position[i] = position;
        lights.color[i] = color;
        lights.attenuation[i] = attenuation;
        lights.spotCutoff[i] = spotCutoff;
    }

    void update_shader_lights(ShaderProgram* shader) {
        if (shader) {
            shader->setUniform("active_lights", this->active_lights);

            GLint loc = shader->getUniformLocation("lights.position");
            glUniform4fv(loc, MAX_LIGHTS, glm::value_ptr(lights.position[0]));

            loc = shader->getUniformLocation("lights.color");
            glUniform4fv(loc, MAX_LIGHTS, glm::value_ptr(lights.color[0]));

            loc = shader->getUniformLocation("lights.attenuation");
            glUniform4fv(loc, MAX_LIGHTS, &lights.attenuation[0]);

            loc = shader->getUniformLocation("lights.spotCutoff");
            glUniform4fv(loc, MAX_LIGHTS, &lights.spotCutoff[0]);
        }
    }
};
