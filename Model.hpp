#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <memory> 

#include <GL/glew.h>
#include <glm/glm.hpp> 

#include "assets.hpp"
#include "Mesh.hpp"
#include "ShaderProgram.hpp"

class Model {
public:
    // origin point of whole model
    glm::vec3 pivot_position{}; // [0,0,0] of the object
    glm::vec3 eulerAngles{};    // pitch, yaw, roll
    glm::vec3 scale{ 1.0f };

    bool parameters_modified{true};
    glm::mat4 local_model_matrix{ 1.0 }; //cache, and for complex transformations (default = identity) 

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

    // mesh related data
    struct mesh_package {
        std::shared_ptr<Mesh> mesh;         // geometry & topology, vertex attributes
        std::shared_ptr<ShaderProgram> shader;     // which shader to use to draw this part of the model

        glm::vec3 origin;                   // mesh origin relative to origin of the whole model
        glm::vec3 eulerAngles;              // mesh rotation relative to orientation of the whole model
        glm::vec3 scale;                    // mesh scale relative to scale of the whole model
    };

    std::vector<mesh_package> meshes;

    Model() = default;
    Model(const std::filesystem::path& filename, std::shared_ptr<ShaderProgram> shader) {
        // Load mesh (all meshes) of the model, (in the future: load material of each mesh, load textures...)
        // notice: you can load multiple meshes and place them to proper positions, 
        //            multiple textures (with reusing) etc. to construct single complicated Model   
        //
        // This can be done by extending OBJ file parser (OBJ can load hierarchical models),
        // or by your own JSON model specification (or keep it simple and set a rule: 1model=1mesh ...) 
        //
    }

    void addMesh(std::shared_ptr<Mesh> mesh,
        std::shared_ptr<ShaderProgram> shader,
        glm::vec3 origin = glm::vec3(0.0f),      // dafault value
        glm::vec3 eulerAngles = glm::vec3(0.0f), // dafault value
        glm::vec3 scale = glm::vec3(1.0f)       // dafault value
    ) {
        meshes.emplace_back(mesh, shader, origin, eulerAngles, scale);
    }

    // ### NEW  (similar can be created for Mesh class)
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

    void draw() {
        if (parameters_modified) {
            force_mm_update();
        }

        // call draw() on mesh (all meshes)
        for (auto const& mesh_pkg : meshes) {
            mesh_pkg.shader->use(); // select proper shader

            //## NEW: calculate and set model matrix 
            glm::mat4 mesh_model_matrix = createMM(mesh_pkg.origin, mesh_pkg.eulerAngles, mesh_pkg.scale);        
            mesh_pkg.shader->setUniform("uM_m", mesh_model_matrix * local_model_matrix);   

            mesh_pkg.mesh->draw();   // draw mesh
        }
    }
};

