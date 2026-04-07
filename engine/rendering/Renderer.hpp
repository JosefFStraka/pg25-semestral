#pragma once

#include <memory>

#include "../../Scene.hpp"
#include "../resources/ResourceManager.hpp"

#include <GL/glew.h>
#include <glm/glm.hpp>

class Renderer {
public:
    void render(ResourceManager* resourceManager, Scene* scene) {
        for (auto& [name, modelInst] : scene->models) {
            modelInst.prepare();

            auto modelRes = resourceManager->getModel(modelInst.model);
            if (!modelRes) continue;

            for (auto const& meshPkg : modelRes->meshes) {
                auto mesh = resourceManager->getMesh(meshPkg.mesh);
                if (!mesh) continue;
                auto shader = resourceManager->getShader(meshPkg.shader);
                if (!shader) continue;
                auto tex = resourceManager->getTexture(meshPkg.texture);
                if (!tex) continue;

                shader->use();
                glm::mat4 mesh_model_matrix = modelInst.createMM(meshPkg.origin, meshPkg.eulerAngles, meshPkg.scale);
                shader->setUniform("uM_m", mesh_model_matrix * modelInst.local_model_matrix);

                tex->bind();
                //shader->setUniform("tex0", 0);

                drawMesh(mesh);
            }
        }

        drawSkybox(resourceManager, scene->skybox);
    }

    void drawSkybox(ResourceManager* resourceManager, Skybox* skybox) {
        if (!skybox)
            return;

        auto mesh = resourceManager->getMesh(skybox->mesh_);
        auto shader = resourceManager->getShader(skybox->shader_);

        shader->use();
        glBindTextureUnit(7, skybox->cubemap_->getName());
        shader->setUniform("skybox", 7);

        glDepthFunc(GL_LEQUAL);
        drawMesh(mesh);
        glDepthFunc(GL_LESS);
    }

    void drawMesh(Mesh* mesh) {
        if (!mesh)
            return;

        mesh->bind();
        if (mesh->hasEbo()) {
            glDrawElements(mesh->getPrimitiveType(), mesh->getIndexCount(), GL_UNSIGNED_INT, nullptr);
        } else {
            glDrawArrays(mesh->getPrimitiveType(), 0, mesh->getVertexCount());
        }
    }
};
