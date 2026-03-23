#pragma once

#ifndef __assets_h__
#define __assets_h__

#include <GL/glew.h> 
#include <GL/wglew.h> 
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

//Vertex description
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;

    bool operator == (const Vertex& v1) const {
        return (position == v1.position
            && normal == v1.normal
            && texCoords == v1.texCoords);
    }
};

// Hash function for glm::vec types
struct VertexHasher {
    size_t operator()(const Vertex& v) const {
        auto hashVec3 = [](const glm::vec3& vec) -> size_t {
            size_t h1 = std::hash<float>()(vec.x);
            size_t h2 = std::hash<float>()(vec.y);
            size_t h3 = std::hash<float>()(vec.z);
            return ((h1 ^ (h2 << 1)) >> 1) ^ h3;
        };
        auto hashVec2 = [](const glm::vec2& vec) -> size_t {
            size_t h1 = std::hash<float>()(vec.x);
            size_t h2 = std::hash<float>()(vec.y);
            return h1 ^ (h2 << 1);
        };

        size_t h = hashVec3(v.position);
        h ^= hashVec3(v.normal) + 0x9e3779b9 + (h << 6) + (h >> 2); // boost-like hash combine
        h ^= hashVec2(v.texCoords) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

#endif
