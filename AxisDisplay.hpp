#pragma once

#include "engine/resources/AssetManager.hpp"
#include "engine/rendering/Renderer.hpp"
#include "ModelInstance.hpp"
#include "ModelResource.hpp"
#include "camera.hpp"
#include "meshgen.hpp"

class AxisDisplay {
    ResourceHandle<Mesh> mesh_;

    ResourceHandle<ShaderProgram> shader_;

    ResourceHandle<Texture> texRed;
    ResourceHandle<Texture> texGreen;
    ResourceHandle<Texture> texBlue;
    ResourceHandle<Texture> texWhite;

    AssetManager* assets;
    Renderer renderer;
    Scene scene;

    ResourceHandle<ModelResource> createSimpleModel(
        AssetManager* rm,
        const std::string& name,
        ResourceHandle<Mesh> mesh,
        ResourceHandle<Texture> texture,
        ResourceHandle<ShaderProgram> shader) {
        ModelResource res;

        res.addMesh(mesh, texture, shader);

        return rm->emplaceModel(name, std::move(res));
    }
    void init_model(std::string name, ResourceHandle<Texture> texture, glm::vec3 trans, glm::vec3 scale) {
        auto modelHandle = createSimpleModel(
            assets,
            name,
            mesh_,
            texture,
            shader_
        );

        ModelInstance modelInstance;
        modelInstance.model = modelHandle;
        modelInstance.setPosition(trans);
        modelInstance.setScale(scale);

        scene.models.emplace(name, modelInstance);
    }

    void update_camera(std::shared_ptr<Camera> other) {
        auto cam = scene.camera;

        cam->Position = other->Position;
        cam->Front = other->Front;
        cam->Up = other->Up;
    }
public:
    GLint viewport[4];

    void init(AssetManager* am) {
        assets = am;
        mesh_ = assets->getHandle<Mesh>("cube");
        texRed = assets->addProgrammatic("axis_red", std::make_unique<Texture>(glm::vec3(1.f, 0.f, 0.f)));
        texGreen = assets->addProgrammatic("axis_green", std::make_unique<Texture>(glm::vec3(0.f, 1.f, 0.f)));
        texBlue = assets->addProgrammatic("axis_blue", std::make_unique<Texture>(glm::vec3(0.f, 0.f, 1.f)));
        texWhite = assets->getHandle<Texture>("white");
        shader_ = assets->addProgrammatic("axis_shader", std::make_unique<ShaderProgram>("../engine/assets/shaders/tex.vert", "../engine/assets/shaders/tex.frag", false));
        
        // OpenGL uses a right-handed coordinate system where
        // the positive x-axis points to the right, 
        // the positive y-axis points up, and 
        // the positive z-axis points out of the screen towards the viewer.
        auto right = glm::vec3(1.f, 0.f, 0.f);
        auto up = glm::vec3(0.f, 1.f, 0.f);
        auto front = glm::vec3(0.f, 0.f, 1.f);
        
        glm::vec3 scale(0.4f, 0.4f, 0.4f);
        float direction_scale = 0.5f;

        init_model("axisXModel", texRed, front * direction_scale, scale);
        init_model("axisYModel", texGreen, up * direction_scale, scale);
        init_model("axisZModel", texBlue, right * direction_scale, scale);
        init_model("centerModel", texWhite, glm::vec3(0.f, 0.f, 0.f), scale * 1.5f);
    }

    void set_viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
        viewport[0] = x;
        viewport[1] = y;
        viewport[2] = width;
        viewport[3] = height;
    }

    void draw(std::shared_ptr<Camera> camera) {
        glClear(GL_DEPTH_BUFFER_BIT);
        GLint backup_viewport[4];
        glGetIntegerv(GL_VIEWPORT, backup_viewport);
        GLboolean backup_cullface;
        glGetBooleanv(GL_CULL_FACE, &backup_cullface);

        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        glDisable(GL_CULL_FACE);

        auto s = assets->getResource(shader_);
        if (s) {
            s->setUniform("uV_m", glm::lookAt(-camera->Front, glm::vec3(0.f, 0.f, 0.f), camera->Up));
            s->setUniform("uP_m", glm::ortho(-1.2f, 1.2f, -1.2f, 1.2f, -0.2f, 2.2f));
        }

        glLineWidth(1); // line widt greater than 1 is depracated

        renderer.render(assets, &scene);

        glViewport(backup_viewport[0], backup_viewport[1], backup_viewport[2], backup_viewport[3]);
        if (backup_cullface) {
            glEnable(GL_CULL_FACE);
        }
    }
};
