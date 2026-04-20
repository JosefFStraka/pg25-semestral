#pragma once

#include <memory>
#include <unordered_set>

#include "../../Scene.hpp"
#include "../resources/AssetManager.hpp"

#include <GL/glew.h>
#include <glm/glm.hpp>

class Renderer {
    std::vector<ModelInstance*> transparent;
    std::unordered_set<ResourceHandle<ShaderProgram>> shaders;

public:
    bool should_update_vp = true;
    int mesh_count = 0;

    bool depth_test{ true };
    bool cull_face{ true };

    void render(AssetManager* assetManager, Scene* scene) {
        mesh_count = 0;
        shaders.clear();
        shaders.reserve(scene->models.size());
        transparent.clear();
        transparent.reserve(scene->models.size());

        if (depth_test)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);

        if (cull_face)
            glEnable(GL_CULL_FACE);
        else
            glDisable(GL_CULL_FACE);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        for (auto& [name, modelInst] : scene->models) {
            if (!modelInst.enabled) continue;
            modelInst.prepare();

            if (modelInst.is_transparent) {
                transparent.push_back(&modelInst);
                continue;
            }

            auto modelRes = assetManager->getResource(modelInst.model);
            if (!modelRes) continue;

            for (auto const& meshPkg : modelRes->meshes) {
                drawMeshPkg(assetManager, scene, &modelInst, meshPkg);
            }
        }

        std::sort(transparent.begin(), transparent.end(), [&](ModelInstance* const a, ModelInstance* const b) {
            return glm::distance(scene->camera->Position, a->getPosition()) > glm::distance(scene->camera->Position, b->getPosition());
            });

        drawSkybox(assetManager, scene->skybox.get(), scene->camera.get());

        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);

        for (auto p : transparent) {
            auto modelRes = assetManager->getResource(p->model);
            if (!modelRes) continue;

            for (auto const& meshPkg : modelRes->meshes) {
                drawMeshPkg(assetManager, scene, p, meshPkg);
            }
        }

        glDepthMask(GL_TRUE);
    }

    void drawSkybox(AssetManager* assetManager, Skybox* skybox, Camera* cam) {
        if (!skybox || !cam)
            return;

        auto mesh = assetManager->getResource(skybox->mesh_);
        auto shader = assetManager->getResource(skybox->shader_);

        if (!mesh || !shader) return;

        shader->use();
        glBindTextureUnit(7, skybox->cubemap_->getName());
        shader->setUniform("skybox", 7);
        shader->setUniform("uV_m", cam->GetViewMatrix());
        shader->setUniform("uP_m", cam->GetProjMatrix());

        glDepthFunc(GL_LEQUAL);
        drawMesh(mesh, cam, nullptr);
        glDepthFunc(GL_LESS);
    }

    void drawMeshPkg(AssetManager* assetManager, Scene* scene, ModelInstance* modelInst, const ModelResource::MeshPackage& meshPkg) {
        auto mesh = assetManager->getResource(meshPkg.mesh);
        if (!mesh) return;
        auto shader = assetManager->getResource(meshPkg.shader);
        if (!shader) return;

        if (scene->camera && !shaders.contains(meshPkg.shader)) {
            shaders.insert(meshPkg.shader);

            shader->setUniform("uV_m", scene->camera->GetViewMatrix());
            shader->setUniform("uP_m", scene->camera->GetProjMatrix());
            shader->setUniform("uAlpha", modelInst->opacity);
        }

        auto tex = assetManager->getResource(meshPkg.texture);
        if (!tex) return;

        shader->use();
        glm::mat4 mesh_model_matrix = modelInst->createMM(meshPkg.origin, meshPkg.eulerAngles, meshPkg.scale);
        glm::mat4 mm = mesh_model_matrix * modelInst->local_model_matrix;
        shader->setUniform("uM_m", mm);

        tex->bind();
        //shader->setUniform("tex0", 0);

        drawMesh(mesh, scene->camera.get(), &mm);
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
