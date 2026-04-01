#pragma once
#include <unordered_map>
#include <iostream>
#include <memory>

#include "ResourceEntry.hpp"
#include "ResourceStorage.hpp"
#include "../rendering/Texture.hpp"
#include "../rendering/Mesh.hpp"
#include "../rendering/ShaderProgram.hpp"
#include "../OBJloader.hpp"

class ResourceManager {
public:
    ResourceManager() {
        textures_.setLoader([](const std::string& path) {
            return std::make_shared<Texture>(path);
            });
        meshes_.setLoader([](const std::string& path) {
            std::vector<Vertex> vertices;
            std::vector<GLuint> indices;
            if (!loadOBJ(path, vertices, indices)) {
                std::cout << "Loading failed: " << path << std::endl;
            }

            return std::make_shared<Mesh>(vertices, indices, GL_TRIANGLES);
            });
    }

    // ==== TEXTURES ====
    ResourceHandle<Texture> registerTexture(std::string const& path) {
        return textures_.create({ path, nullptr });
    }
    template<typename... Args>
    ResourceHandle<Texture> emplaceTexture(Args&&... args) {
        return textures_.emplace("", std::forward<Args>(args)...);
    }
    std::shared_ptr<Texture> getTexture(ResourceHandle<Texture> handle) {
        return textures_.get(handle);
    }

    // ==== MESH ====
    ResourceHandle<Mesh> registerMesh(std::string const& path) {
        return meshes_.create({ path, nullptr });
    }
    template<typename... Args>
    ResourceHandle<Mesh> emplaceMesh(Args&&... args) {
        return meshes_.emplace("", std::forward<Args>(args)...);
    }
    ResourceHandle<Mesh> addSharedMesh(std::string const& path, std::shared_ptr<Mesh> m) {
        return meshes_.create({ path, m });
    }
    std::shared_ptr<Mesh> getMesh(ResourceHandle<Mesh> handle) {
        return meshes_.get(handle);
    }
private:
    ResourceStorage<Texture> textures_;
    ResourceStorage<Mesh> meshes_;
};
