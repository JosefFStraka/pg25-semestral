#pragma once
#include <unordered_map>
#include <iostream>
#include <memory>

#include "resource_entry.hpp"
#include "resource_handle.hpp"
#include "../rendering/Texture.hpp"
#include "../rendering/Mesh.hpp"
#include "../rendering/ShaderProgram.hpp"
#include "../OBJloader.hpp"

class resource_manager {
public:
    // ==== TEXTURES ====
    resource_handle<Texture> register_texture(std::string const& path) {
        textures_.emplace(++texture_resource_id, resource_entry<Texture>{path, std::nullopt});
        return resource_handle<Texture>{texture_resource_id};
    }
    std::shared_ptr<Texture> get_texture(resource_handle<Texture> handle) {

        auto it = textures_.find(handle.get_id());
        resource_entry<Texture>* res = &textures_.at(1);
        if (it == textures_.end()) {
            std::cout << "Couldnt find texture id:" << handle.get_id() << std::endl;
        } else {
            res = &it->second;
        }

        if (!res->data.has_value()) {
            res->data = std::make_shared<Texture>(res->path);
        }

        return res->data.value();
    }

    // ==== MESH ====
    resource_handle<Mesh> register_mesh(std::string const& path) {
        meshes_.emplace(++mesh_resource_id, resource_entry<Mesh>{path, std::nullopt});
        return resource_handle<Mesh>{mesh_resource_id};
    }
    resource_handle<Mesh> add_mesh(std::vector<Vertex> const& vertices, GLenum primitive_type) {
        meshes_.emplace(
            ++mesh_resource_id,
            resource_entry<Mesh>{"", std::make_shared<Mesh>(vertices, primitive_type)}
        );
        return resource_handle<Mesh>{mesh_resource_id};
    }
    resource_handle<Mesh> add_mesh(std::shared_ptr<Mesh> mesh) {
        meshes_.emplace(
            ++mesh_resource_id,
            resource_entry<Mesh>{"", mesh}
        );
        return resource_handle<Mesh>{mesh_resource_id};
    }
    std::shared_ptr<Mesh> get_mesh(resource_handle<Mesh> handle) {
        auto it = meshes_.find(handle.get_id());
        resource_entry<Mesh>* res = &meshes_.at(1);
        if (it == meshes_.end()) {
            std::cout << "Couldnt find mesh id:" << handle.get_id() << std::endl;
        } else {
            res = &it->second;
        }

        if (!res->data.has_value()) {
            std::vector<Vertex> vertices;
            std::vector<GLuint> indices;
            if (!loadOBJ(res->path, vertices, indices)) {
                std::cout << "Loading failed: " << res->path << std::endl;
            }

            res->data = std::make_shared<Mesh>(vertices, indices, GL_TRIANGLES);
        }

        return res->data.value();
    }

    // resource_handle<ShaderProgram> register_shader(std::string const& path) {
    //     shaders_.emplace(++shader_resource_id, resource_entry<Texture>{path, nullptr});
    //     return resource_handle<ShaderProgram>{shader_resource_id};
    // }
    // std::shared_ptr<ShaderProgram> get_shader(resource_handle<ShaderProgram> handle) {

    //     auto it = shaders_.find(handle.get_id());
    //     resource_entry<ShaderProgram>* res = &shaders_.at(1);
    //     if (it == shaders_.end()) {
    //         std::cout << "Couldnt find shader id:" << handle.get_id() << std::endl;
    //     } else {
    //         res = &it->second;
    //     }

    //     if (!res->data) {
    //         res->data = std::make_shared<ShaderProgram>(res->path + ".vert", res->path + ".frag", false);
    //     }

    //     return res->data;
    // }

private:

    resource_id texture_resource_id{ 0 };
    std::unordered_map<resource_id, resource_entry<Texture>> textures_;

    resource_id mesh_resource_id{ 0 };
    std::unordered_map<resource_id, resource_entry<Mesh>> meshes_;

    // resource_id shader_resource_id{ 0 };
    // std::unordered_map<resource_id, resource_entry<ShaderProgram>> shaders_;
};
