#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "engine/resources/ResourceHandle.hpp"
#include "engine/rendering/Mesh.hpp"
#include "engine/rendering/Texture.hpp"
#include "engine/rendering/ShaderProgram.hpp"

struct ModelResource {
    struct MeshPackage {
        ResourceHandle<Mesh> mesh;
        ResourceHandle<Texture> texture;
        ResourceHandle<ShaderProgram> shader;

        glm::vec3 origin;
        glm::vec3 eulerAngles;
        glm::vec3 scale;
    };

    std::vector<MeshPackage> meshes;


    void addMesh(
        ResourceHandle<Mesh> mesh,
        ResourceHandle<Texture> texture,
        ResourceHandle<ShaderProgram> shader,
        glm::vec3 origin = glm::vec3(0.0f),      // dafault value
        glm::vec3 eulerAngles = glm::vec3(0.0f), // dafault value
        glm::vec3 scale = glm::vec3(1.0f)       // dafault value
    ) {
        meshes.emplace_back(mesh, texture, shader, origin, eulerAngles, scale);
    }
};
