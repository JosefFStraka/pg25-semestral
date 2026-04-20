#pragma once

#include <memory> 
#include <GL/glew.h> 
#include <GL/wglew.h> 
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#define FROM(name) t.name = j.value(#name, t.name)
#define TO(name) {#name, t.name}

namespace glm {

    inline void to_json(json& j, const vec3& v) {
        j = json::array({ v.x, v.y, v.z });
    }

    inline void from_json(const json& j, vec3& v) {
        v.x = j.at(0).get<float>();
        v.y = j.at(1).get<float>();
        v.z = j.at(2).get<float>();
    }
    inline void to_json(json& j, const vec2& v) {
        j = json::array({ v.x, v.y });
    }

    inline void from_json(const json& j, vec2& v) {
        v.x = j.at(0).get<float>();
        v.y = j.at(1).get<float>();
    }
}

//Vertex description
struct Vertex {
    glm::vec3 position{ 0.f,0.f,0.f };
    glm::vec3 normal{ 0.f,0.f,0.f };
    glm::vec2 texCoords{ 0.f,0.f };

    bool operator == (const Vertex& v1) const {
        return (position == v1.position
            && normal == v1.normal
            && texCoords == v1.texCoords);
    }
};

inline void to_json(json& j, const Vertex& t) {
    j = json{
        {"position", json(t.position)},
        {"normal", json(t.normal)},
        {"texCoords", json(t.texCoords)}
    };
}

inline void from_json(const json& j, Vertex& t) {
    t.position  = j.at("position").get<glm::vec3>();
    t.normal    = j.at("normal").get<glm::vec3>();
    t.texCoords = j.at("texCoords").get<glm::vec2>();
}

namespace nlohmann {
    template <typename T>
    struct adl_serializer<std::unique_ptr<T>> {
        static void to_json(json& j, const std::unique_ptr<T>& opt) {
            if (opt.get()) {
                j = *opt;
            } else {
                j = nullptr;
            }
        }
    };
}
