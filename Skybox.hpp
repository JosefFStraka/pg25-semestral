#pragma once
#include <GL/glew.h> 
#include <glm/glm.hpp>

#include "engine/resources/ResourceHandle.hpp"
#include "engine/rendering/Mesh.hpp"

#include "CubeMapTexture.hpp"
#include "ShaderProgram.hpp"

class Skybox {
public:
    ResourceHandle<Mesh> mesh_;
    CubeMapTexture* cubemap_;
    ResourceHandle<ShaderProgram> shader_;

    Skybox(ResourceHandle<Mesh> mesh, CubeMapTexture* cubemap, ResourceHandle<ShaderProgram> shader) {
        mesh_ = mesh;
        cubemap_ = cubemap;
        shader_ = shader;
    }
    ~Skybox() {
        if (cubemap_) delete cubemap_;
    }
};
