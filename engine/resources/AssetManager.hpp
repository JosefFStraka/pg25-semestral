#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <functional>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <tuple>

#include <nlohmann/json.hpp>

#include "ResourceHandle.hpp"
#include "ResourceEntry.hpp"
#include "engine/rendering/Mesh.hpp"
#include "engine/rendering/Texture.hpp"
#include "engine/rendering/ShaderProgram.hpp"

#include "../../ModelResource.hpp"
#include "../../OBJloader.hpp"
#include "../../image_io.hpp"

using json = nlohmann::json;

class AssetManager {
private:
    struct GPUJob {
        std::function<void()> job;
    };
    std::vector<GPUJob> gpu_queue_;
    std::mutex gpu_queue_mutex_;

    template<typename T>
    struct Storage {
        ResourceId last_id = 1;
        std::unordered_map<std::string, ResourceId> name_to_id;
        std::unordered_map<ResourceId, ResourceEntry<T>> entries;
    };

    Storage<Texture> textures_;
    Storage<Mesh> meshes_;
    Storage<ShaderProgram> shaders_;
    Storage<ModelResource> models_;

    std::string config_path;

    void queue_gpu_job(std::function<void()>&& job) {
        std::lock_guard<std::mutex> lock(gpu_queue_mutex_);
        gpu_queue_.push_back({std::move(job)});
    }

public:
    AssetManager() = default;

    int load(const std::string& path) {
        config_path = path;

        if (!std::filesystem::exists(path)) {
            std::cerr << "AssetManager::load - File does not exist: " << path << std::endl;
            return 1;
        }

        std::ifstream file(path);
        json j;
        file >> j;

        if (j.contains("textures")) {
            for (auto& [name, desc] : j["textures"].items()) {
                auto id = textures_.last_id++;
                textures_.name_to_id[name] = id;
                textures_.entries[id].path = desc["path"].get<std::string>();
                textures_.entries[id].state = ResourceEntry<Texture>::None;
            }
        }
        if (j.contains("meshes")) {
            for (auto& [name, desc] : j["meshes"].items()) {
                auto id = meshes_.last_id++;
                meshes_.name_to_id[name] = id;
                meshes_.entries[id].path = desc["path"].get<std::string>();
                meshes_.entries[id].state = ResourceEntry<Mesh>::None;
                
                if (desc.contains("aabb_min") && desc.contains("aabb_max")) {
                    AABB parsed_aabb;
                    parsed_aabb.min = glm::vec3(desc["aabb_min"][0], desc["aabb_min"][1], desc["aabb_min"][2]);
                    parsed_aabb.max = glm::vec3(desc["aabb_max"][0], desc["aabb_max"][1], desc["aabb_max"][2]);
                    meshes_.entries[id].aabb = parsed_aabb;
                }
            }
        }
        if (j.contains("shaders")) {
            for (auto& [name, desc] : j["shaders"].items()) {
                auto id = shaders_.last_id++;
                shaders_.name_to_id[name] = id;
                shaders_.entries[id].path = desc["vs"].get<std::string>() + "|" + desc["fs"].get<std::string>();
                shaders_.entries[id].state = ResourceEntry<ShaderProgram>::None;
            }
        }

        return 0;
    }

    void save_assets() {
        if (config_path.empty()) return;
        
        std::ifstream in_file(config_path);
        json j;
        if (in_file.is_open()) {
            in_file >> j;
            in_file.close();
        }

        if (j.contains("meshes")) {
            for (auto& [name, desc] : j["meshes"].items()) {
                if (meshes_.name_to_id.contains(name)) {
                    auto id = meshes_.name_to_id[name];
                    if (meshes_.entries[id].aabb.has_value()) {
                        auto& aabb = meshes_.entries[id].aabb.value();
                        desc["aabb_min"] = { aabb.min.x, aabb.min.y, aabb.min.z };
                        desc["aabb_max"] = { aabb.max.x, aabb.max.y, aabb.max.z };
                    }
                }
            }
        }

        std::ofstream out_file(config_path);
        if (out_file.is_open()) {
            out_file << j.dump(2);
        }
    }

    void update() {
        std::vector<GPUJob> jobs;
        {
            std::lock_guard<std::mutex> lock(gpu_queue_mutex_);
            jobs = std::move(gpu_queue_);
            gpu_queue_.clear();
        }
        for (auto& job : jobs) {
            job.job();
        }
    }

    template<typename T>
    ResourceHandle<T> getHandle(const std::string& name) {
        if constexpr (std::is_same_v<T, Texture>) {
            if (textures_.name_to_id.contains(name)) return ResourceHandle<T>(textures_.name_to_id[name]);
        } else if constexpr (std::is_same_v<T, Mesh>) {
            if (meshes_.name_to_id.contains(name)) return ResourceHandle<T>(meshes_.name_to_id[name]);
        } else if constexpr (std::is_same_v<T, ShaderProgram>) {
            if (shaders_.name_to_id.contains(name)) return ResourceHandle<T>(shaders_.name_to_id[name]);
        }else if constexpr (std::is_same_v<T, ModelResource>) {
            if (models_.name_to_id.contains(name)) return ResourceHandle<T>(models_.name_to_id[name]);
        }
        return ResourceHandle<T>(0);
    }

    template<typename T>
    ResourceHandle<T> emplaceModel(const std::string& name, T&& model) {
        auto id = models_.last_id++;
        models_.name_to_id[name] = id;
        models_.entries[id].path = name;
        models_.entries[id].data = std::make_unique<T>(std::move(model));
        models_.entries[id].state = ResourceEntry<T>::Ready;
        return ResourceHandle<T>(id);
    }

    template<typename T>
    ResourceHandle<T> addProgrammatic(const std::string& name, std::unique_ptr<T> obj) {
        if constexpr (std::is_same_v<T, Texture>) {
            auto id = textures_.last_id++;
            textures_.name_to_id[name] = id;
            textures_.entries[id].data = std::move(obj);
            textures_.entries[id].state = ResourceEntry<T>::Ready;
            return ResourceHandle<T>(id);
        } else if constexpr (std::is_same_v<T, Mesh>) {
            auto id = meshes_.last_id++;
            meshes_.name_to_id[name] = id;
            meshes_.entries[id].data = std::move(obj);
            meshes_.entries[id].state = ResourceEntry<T>::Ready;
            return ResourceHandle<T>(id);
        } else if constexpr (std::is_same_v<T, ShaderProgram>) {
            auto id = shaders_.last_id++;
            shaders_.name_to_id[name] = id;
            shaders_.entries[id].data = std::move(obj);
            shaders_.entries[id].state = ResourceEntry<T>::Ready;
            return ResourceHandle<T>(id);
        }
        return ResourceHandle<T>(0);
    }

    template<typename T>
    T* getResource(ResourceHandle<T> handle) {
        if (handle.get_id() == 0) return nullptr;

        Storage<T>* storage = nullptr;
        if constexpr (std::is_same_v<T, Texture>) storage = (Storage<T>*) &textures_;
        else if constexpr (std::is_same_v<T, Mesh>) storage = (Storage<T>*) &meshes_;
        else if constexpr (std::is_same_v<T, ShaderProgram>) storage = (Storage<T>*) &shaders_;
        else if constexpr (std::is_same_v<T, ModelResource>) storage = (Storage<T>*) &models_;

        if (!storage) return nullptr;
        
        auto it = storage->entries.find(handle.get_id());
        if (it == storage->entries.end()) return nullptr;

        auto& entry = it->second;

        if (entry.state == ResourceEntry<T>::Ready) {
            return entry.data.get();
        }

        if (entry.state == ResourceEntry<T>::None) {
            entry.state = ResourceEntry<T>::Loading;

            if constexpr (std::is_same_v<T, Mesh>) {
                std::string path = entry.path;
                auto id = handle.get_id();
                std::optional<AABB> predefined_aabb = entry.aabb;
                
                std::thread([this, path, id, predefined_aabb]() {
                    std::vector<Vertex> vertices;
                    std::vector<GLuint> indices;
                    AABB obj_aabb;
                    
                    if (!loadOBJ(path, vertices, indices, obj_aabb)) {
                        std::cerr << "Failed to load mesh: " << path << std::endl;
                        queue_gpu_job([this, id]() {
                            meshes_.entries[id].state = ResourceEntry<Mesh>::Error;
                        });
                        return;
                    }

                    AABB final_aabb = predefined_aabb.has_value() ? predefined_aabb.value() : obj_aabb;

                    if (!predefined_aabb.has_value()) {
                        queue_gpu_job([this, id, final_aabb]() {
                            this->meshes_.entries[id].aabb = final_aabb;
                            this->save_assets();
                        });
                    }

                    auto md = std::make_shared<std::tuple<std::vector<Vertex>, std::vector<GLuint>, AABB>>(
                        std::move(vertices), std::move(indices), final_aabb
                    );

                    queue_gpu_job([this, id, md]() {
                        auto& entry = meshes_.entries[id];
                        entry.data = std::make_unique<Mesh>(std::get<0>(*md), std::get<1>(*md), GL_TRIANGLES);
                        entry.data->aabb_ = std::get<2>(*md);
                        entry.state = ResourceEntry<Mesh>::Ready;
                    });
                }).detach();
            } else if constexpr (std::is_same_v<T, Texture>) {
                std::string path = entry.path;
                auto id = handle.get_id();
                
                std::thread([this, path, id]() {
                    try {
                        auto mat = std::make_shared<GLMat>(imread(path, 1));
                        queue_gpu_job([this, id, mat]() {
                            auto& entry = textures_.entries[id];
                            entry.data = std::make_unique<Texture>(*mat);
                            entry.state = ResourceEntry<Texture>::Ready;
                        });
                    } catch (const std::exception& e) {
                        std::cerr << "Failed to load texture: " << path << " (" << e.what() << ")" << std::endl;
                        queue_gpu_job([this, id]() {
                            textures_.entries[id].state = ResourceEntry<Texture>::Error;
                        });
                    }
                }).detach();
            } else if constexpr (std::is_same_v<T, ShaderProgram>) {
                std::string path = entry.path;
                auto id = handle.get_id();
                
                queue_gpu_job([this, id, path]() {
                    auto delim_pos = path.find('|');
                    if (delim_pos != std::string::npos) {
                        std::string vs = path.substr(0, delim_pos);
                        std::string fs = path.substr(delim_pos + 1);
                        auto& entry = shaders_.entries[id];
                        entry.data = std::make_unique<ShaderProgram>(vs, fs, false);
                        entry.state = ResourceEntry<ShaderProgram>::Ready;
                    } else {
                        shaders_.entries[id].state = ResourceEntry<ShaderProgram>::Error;
                    }
                });
            } else if constexpr (std::is_same_v<T, ModelResource>) {
                entry.state = ResourceEntry<ModelResource>::Error;
            }
        }

        return nullptr;
    }
};
