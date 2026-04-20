#pragma once
#include <glm/glm.hpp> 
#include "aabb.hpp"

struct Plane {
    glm::vec3 normal{0.f, 1.f, 0.f};
    float distance{0.f};

    // check if AABB is entirely IN FRONT of the plane or intersecting
    // Returns true if the AABB is on the side of the normal (forward)
    bool isAABB_forward(const AABB& aabb) const {
        glm::vec3 extents = (aabb.max - aabb.min) * 0.5f;
        glm::vec3 center = aabb.min + extents;
        float r = extents.x * std::abs(normal.x) + 
                  extents.y * std::abs(normal.y) + 
                  extents.z * std::abs(normal.z);
        return -r <= (glm::dot(normal, center) + distance);
    }
};