#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <memory> 
#include <optional> 

#include "assets.hpp"
#include "engine/resources/ResourceHandle.hpp"
#include "engine/rendering/Mesh.hpp"
#include "engine/rendering/Texture.hpp"
#include "engine/rendering/ShaderProgram.hpp"

#include "ModelResource.hpp"

#include <GL/glew.h>
#include <glm/glm.hpp> 

class ModelInstance {
public:
    ResourceHandle<ModelResource> model;

    // origin point of whole model
    glm::vec3 pivot_position{}; // [0,0,0] of the object
    glm::vec3 eulerAngles{};    // pitch, yaw, roll
    glm::vec3 scale{ 1.0f };

    bool parameters_modified{ true };
    glm::mat4 local_model_matrix{ 1.0 }; //cache, and for complex transformations (default = identity) 

    //enable for transparent models
    bool is_transparent {false};
    float opacity {1.f};

    // enable/disable collisions for this object
    bool collision_enabled {true};

    glm::mat4 createMM(const glm::vec3& origin, const glm::vec3& eAng, const glm::vec3& scale) {
        // keep angles in proper range
        glm::vec3 eA{ wrapAngle(eAng.x), wrapAngle(eAng.y), wrapAngle(eAng.z) };

        glm::mat4 t = glm::translate(glm::mat4(1.0f), origin);

        //glm::mat4 rotm = glm::yawPitchRoll(glm::radians(eA.y), glm::radians(eA.x), glm::radians(eA.z)); //yaw, pitch, roll
        glm::mat4 rotm =
            glm::rotate(glm::mat4(1.0f), glm::radians(eA.x), glm::vec3(1, 0, 0)) *
            glm::rotate(glm::mat4(1.0f), glm::radians(eA.y), glm::vec3(0, 1, 0)) *
            glm::rotate(glm::mat4(1.0f), glm::radians(eA.z), glm::vec3(0, 0, 1));

        glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);

        return t * rotm * s;
    }

    float wrapAngle(float angle) { // wrap any float to [0, 360)
        angle = std::fmod(angle, 360.0f);
        if (angle < 0.0f) {
            angle += 360.0f;
        }
        return angle;
    }

    void setPosition(const glm::vec3& new_position) {
        pivot_position = new_position;
        parameters_modified = true;
    }

    void setEulerAngles(const glm::vec3& new_eulerAngles) {
        eulerAngles = new_eulerAngles;
        parameters_modified = true;
    }

    void setScale(const glm::vec3& new_scale) {
        scale = new_scale;
        parameters_modified = true;
    }

    // for complex (externally provided) transformations 
    void setModelMatrix(const glm::mat4& modelm) {
        local_model_matrix = modelm;
    }

    void translate(const glm::vec3& offset) {
        pivot_position += offset;
        parameters_modified = true;
    }

    void rotate(const glm::vec3& pitch_yaw_roll_offs) {
        eulerAngles += pitch_yaw_roll_offs;
        eulerAngles.x = wrapAngle(eulerAngles.x);
        eulerAngles.y = wrapAngle(eulerAngles.y);
        eulerAngles.z = wrapAngle(eulerAngles.z);
        parameters_modified = true;
    }

    void doScale(const glm::vec3& scale_offs) {
        scale *= scale_offs;
        parameters_modified = true;
    }

    glm::vec3 getPosition() { 
        // get 3 values from last column of cached model matrix = translation
        return glm::vec3(local_model_matrix[3]);
    }  

    void force_mm_update() {
        local_model_matrix = createMM(pivot_position, eulerAngles, scale);
        parameters_modified = false;
    }

    // update based on running time
    void update(const float delta_t) {
        // change internal state of the model (positions of meshes, size, etc.) 
        // note: this allows dynamic behaviour - it can be modified to 
        //       use lambda funtion, call scripting language, etc. 
    }

    void prepare() {
        if (parameters_modified) {
            force_mm_update();
        }
    }
};
