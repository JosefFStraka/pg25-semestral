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

            for (auto const& meshPkg : modelRes->meshes) {
                auto mesh = resourceManager->getMesh(meshPkg.mesh);
                auto shader = resourceManager->getShader(meshPkg.shader);

                shader->use();
                glm::mat4 mesh_model_matrix = modelInst.createMM(meshPkg.origin, meshPkg.eulerAngles, meshPkg.scale);
                shader->setUniform("uM_m", mesh_model_matrix * modelInst.local_model_matrix);

                auto tex = resourceManager->getTexture(meshPkg.texture);
                tex->bind();
                //shader->setUniform("tex0", 0);

                drawMesh(mesh);
            }
        }
    }
    void drawMesh(Mesh* mesh) {
        mesh->bind();
        if (mesh->hasEbo()) {
            glDrawElements(mesh->getPrimitiveType(), mesh->getIndexCount(), GL_UNSIGNED_INT, nullptr);
        } else {
            glDrawArrays(mesh->getPrimitiveType(), 0, mesh->getVertexCount());
        }
    }
};
