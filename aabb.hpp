#pragma once
#include <glm/glm.hpp>
#include <algorithm>

#undef min
#undef max

struct AABB {
    glm::vec3 min{ 1e30f };
    glm::vec3 max{ -1e30f };

    void expand(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    bool intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }

    AABB transform(const glm::mat4& transform) const {
        glm::vec3 corners[8] = {
            glm::vec3(min.x, min.y, min.z),
            glm::vec3(max.x, min.y, min.z),
            glm::vec3(min.x, max.y, min.z),
            glm::vec3(max.x, max.y, min.z),
            glm::vec3(min.x, min.y, max.z),
            glm::vec3(max.x, min.y, max.z),
            glm::vec3(min.x, max.y, max.z),
            glm::vec3(max.x, max.y, max.z),
        };

        AABB result;
        for (int i = 0; i < 8; i++) {
            glm::vec4 t_corner = transform * glm::vec4(corners[i], 1.0f);
            result.expand(glm::vec3(t_corner));
        }
        return result;
    }
};
