#pragma once

#include <memory>
#include <unordered_set>

#include "../../Scene.hpp"
#include "../resources/ResourceManager.hpp"

#include <GL/glew.h>
#include <glm/glm.hpp>

class Renderer {
public:
    bool should_update_vp = true;
    int mesh_count = 0;

    void render(ResourceManager* resourceManager, Scene* scene) {
        mesh_count = 0;
        std::unordered_set<ResourceHandle<ShaderProgram>> shaders;

        for (auto& [name, modelInst] : scene->models) {
            modelInst.prepare();

            auto modelRes = resourceManager->getModel(modelInst.model);
            if (!modelRes) continue;

            for (auto const& meshPkg : modelRes->meshes) {
                auto mesh = resourceManager->getMesh(meshPkg.mesh);
                if (!mesh) continue;
                auto shader = resourceManager->getShader(meshPkg.shader);
                if (!shader) continue;

                if (scene->camera && !shaders.contains(meshPkg.shader)) {
                    shaders.insert(meshPkg.shader);

                    shader->setUniform("uV_m", scene->camera->GetViewMatrix());
                    shader->setUniform("uP_m", scene->camera->GetProjMatrix());
                }

                auto tex = resourceManager->getTexture(meshPkg.texture);
                if (!tex) continue;

                shader->use();
                glm::mat4 mesh_model_matrix = modelInst.createMM(meshPkg.origin, meshPkg.eulerAngles, meshPkg.scale);
                glm::mat4 mm = mesh_model_matrix * modelInst.local_model_matrix;
                shader->setUniform("uM_m", mm);

                tex->bind();
                //shader->setUniform("tex0", 0);

                drawMesh(mesh, scene->camera.get(), &mm);
            }
        }

        drawSkybox(resourceManager, scene->skybox.get(), scene->camera.get());
    }

    void drawSkybox(ResourceManager* resourceManager, Skybox* skybox, Camera* cam) {
        if (!skybox || !cam)
            return;

        auto mesh = resourceManager->getMesh(skybox->mesh_);
        auto shader = resourceManager->getShader(skybox->shader_);

        shader->use();
        glBindTextureUnit(7, skybox->cubemap_->getName());
        shader->setUniform("skybox", 7);
        shader->setUniform("uV_m", cam->GetViewMatrix());
        shader->setUniform("uP_m", cam->GetProjMatrix());

        glDepthFunc(GL_LEQUAL);
        drawMesh(mesh, cam, nullptr);
        glDepthFunc(GL_LESS);
    }

    void drawMesh(Mesh* mesh, Camera* cam, glm::mat4* mm) {
        if (!mesh)
            return;

        // if (cam && mm) {
        //     auto mat = (*mm) * cam->GetViewMatrix() * cam->GetProjMatrix();
        //     glm::vec4 pos = mat * mesh->bounding_sphere;
        //     if (std::abs(pos.x) > 1 && std::abs(pos.y) > 1 && std::abs(pos.z > 1)) 
        //         return;
        // }


        mesh->bind();
        if (mesh->hasEbo()) {
            glDrawElements(mesh->getPrimitiveType(), mesh->getIndexCount(), GL_UNSIGNED_INT, nullptr);
        } else {
            glDrawArrays(mesh->getPrimitiveType(), 0, mesh->getVertexCount());
        }

        mesh_count++;
    }
};
