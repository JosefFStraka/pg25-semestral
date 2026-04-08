#pragma once
#include <unordered_map>
#include <iostream>
#include <memory>
#include <future>
#include <queue>

#include "ResourceHandle.hpp"
#include "ResourceEntry.hpp"
#include "ResourceStorage.hpp"
#include "../rendering/Texture.hpp"
#include "../rendering/Mesh.hpp"
#include "../rendering/ShaderProgram.hpp"
#include "../OBJloader.hpp"
#include "../../ModelResource.hpp"

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
};

class ResourceManager {
public:
    ResourceManager() {
        textures_.setLoader(
            [](ResourceEntry<Texture>* data, const std::string& path) {
                data->data = std::make_unique<Texture>(path);
                data->state = ResourceEntry<Texture>::state::Ready;
            });
        meshes_.setLoader(
            [this](ResourceEntry<Mesh>* data, const std::string& path) {
                jobs_.push_back(std::async(std::launch::async, [this, data, path]() {
                    MeshData* md = new MeshData();

                    if (!loadOBJ(path, md->vertices, md->indices)) {
                        std::cout << "Failed: " << path << std::endl;
                        return;
                    }

                    std::unique_lock<std::mutex> lock(completedMeshMutex_);
                    completedMesh_.push({ data, md });
                    }));
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
        std::unique_lock<std::mutex> lock(completedMeshMutex_);
        while (!completedMesh_.empty()) {
            auto x = completedMesh_.front();

            x.first->data = std::make_unique<Mesh>(x.second->vertices, x.second->indices, GL_TRIANGLES);
            x.first->state = ResourceEntry<Mesh>::state::Ready;
            delete x.second;
            completedMesh_.pop();
        }

        return models_.get(handle);
    }
private:
    // quick and dirty
    std::vector<std::future<void>> jobs_;
    std::queue<std::pair<ResourceEntry<Mesh>*, MeshData*>> completedMesh_;
    std::mutex completedMeshMutex_;

    ResourceStorage<Texture> textures_;
    ResourceStorage<Mesh> meshes_;
    ResourceStorage<ShaderProgram> shaders_;
    ResourceStorage<ModelResource> models_;
};
