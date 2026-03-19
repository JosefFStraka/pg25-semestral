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

    Vertex() : Vertex(glm::vec3()) {}

    Vertex(glm::vec3 position) {
        this->position = position;
        normal = glm::vec3();
        texCoords = glm::vec3();
    }

    bool operator == (const Vertex& v1) const {
        return (position == v1.position
            && normal == v1.normal
            && texCoords == v1.texCoords);
    }
};

#endif
