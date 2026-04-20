#pragma once
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "assets.hpp"
#include "ResourceDescriptor.hpp"

#define FROM(name) t.name = j.value(#name, t.name)
#define TO(name) {#name, t.name}

using json = nlohmann::json;

class MeshResourceDescriptor : public ResourceDescriptor {
public:
    std::string path;

    MeshResourceDescriptor(std::string path) :
        ResourceDescriptor(ResourceDescriptor::eResourceType::Mesh), path(path) {}

    void to_json(json& j, const MeshResourceDescriptor& t) { 
        j = json({
            TO(type),
            TO(path),
        });
    }
    void from_json(const json& j, MeshResourceDescriptor& t) {
        FROM(type);
        FROM(path);
    }
};
