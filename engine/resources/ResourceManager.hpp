#pragma once
#include <unordered_map>
#include <iostream>
#include <memory>

#include "ResourceHandle.hpp"
#include "ResourceEntry.hpp"
#include "ResourceStorage.hpp"
#include "../rendering/Texture.hpp"
#include "../rendering/Mesh.hpp"
#include "../rendering/ShaderProgram.hpp"
#include "../OBJloader.hpp"
#include "../../ModelResource.hpp"

class ResourceManager {
public:
    ResourceManager() {
        textures_.setLoader(
            [](const std::string& path) {
                return std::make_unique<Texture>(path);
            });
        meshes_.setLoader(
            [](const std::string& path) {
                std::vector<Vertex> vertices;
                std::vector<GLuint> indices;
                if (!loadOBJ(path, vertices, indices)) {
                    std::cout << "Loading failed: " << path << std::endl;
                }

                return std::make_unique<Mesh>(vertices, indices, GL_TRIANGLES);
            });

        //no lazy load for shader yet
    }

    // ==== TEXTURES ====
    ResourceHandle<Texture> registerTexture(std::string const& path) {
        return textures_.create(nullptr, path);
    }
    template<typename... Args>
    ResourceHandle<Texture> emplaceTexture(Args&&... args) {
        return textures_.emplace("", std::forward<Args>(args)...);
    }
    Texture* getTexture(ResourceHandle<Texture> handle) {
        return textures_.get(handle);
    }

    // ==== MESHES ====
    ResourceHandle<Mesh> registerMesh(std::string const& path) {
        return meshes_.create(nullptr, path);
    }
    template<typename... Args>
    ResourceHandle<Mesh> emplaceMesh(Args&&... args) {
        return meshes_.emplace("", std::forward<Args>(args)...);
    }
    Mesh* getMesh(ResourceHandle<Mesh> handle) {
        return meshes_.get(handle);
    }

    // ==== SHADERS ====
    template<typename... Args>
    ResourceHandle<ShaderProgram> emplaceShader(Args&&... args) {
        return shaders_.emplace("", std::forward<Args>(args)...);
    }
    ShaderProgram* getShader(ResourceHandle<ShaderProgram> handle) {
        return shaders_.get(handle);
    }

    // ==== MODELS ====
    // ResourceHandle<ModelResource> registerModel(std::string const& path) {
    //     return models_.create({ path, nullptr });
    // }
    template<typename... Args>
    ResourceHandle<ModelResource> emplaceModel(Args&&... args) {
        return models_.emplace("", std::forward<Args>(args)...);
    }
    ResourceHandle<ModelResource> createModelResource(ModelResource&& model, std::string path = "") {
        return models_.create(std::make_unique<ModelResource>(std::move(model)), path);
    }
    ModelResource* getModel(ResourceHandle<ModelResource> handle) {
        return models_.get(handle);
    }
private:
    ResourceStorage<Texture> textures_;
    ResourceStorage<Mesh> meshes_;
    ResourceStorage<ShaderProgram> shaders_;
    ResourceStorage<ModelResource> models_;
};
