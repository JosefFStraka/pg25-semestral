#pragma once
#include "plane.hpp"
#include "aabb.hpp"
#include <glm/glm.hpp>

struct Frustum {
    Plane planes[6];

    static Frustum extractFrustum(const glm::mat4& m) {
        Frustum f;
        f.planes[0] = { glm::vec3(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0]), m[3][3] + m[3][0] }; // Left
        f.planes[1] = { glm::vec3(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0]), m[3][3] - m[3][0] }; // Right
        f.planes[2] = { glm::vec3(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1]), m[3][3] + m[3][1] }; // Bottom
        f.planes[3] = { glm::vec3(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1]), m[3][3] - m[3][1] }; // Top
        f.planes[4] = { glm::vec3(m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2]), m[3][3] + m[3][2] }; // Near
        f.planes[5] = { glm::vec3(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2]), m[3][3] - m[3][2] }; // Far

        for(int i = 0; i < 6; i++) {
            float length = glm::length(f.planes[i].normal);
            f.planes[i].normal /= length;
            f.planes[i].distance /= length;
        }
        return f;
    }

    bool isAABBInFrustum(const AABB& aabb) const {
        for(int i = 0; i < 6; i++) {
            if (!planes[i].isAABB_forward(aabb)) {
                return false;
            }
        }
        return true;
    }
};
