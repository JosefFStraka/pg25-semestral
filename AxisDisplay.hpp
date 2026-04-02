
#include "engine/resources/ResourceManager.hpp"
#include "engine/rendering/Renderer.hpp"
#include "ModelInstance.hpp"
#include "ModelResource.hpp"
#include "camera.hpp"
#include "meshgen.hpp"

class AxisDisplay {
    ResourceHandle<Mesh> line_;
    ResourceHandle<Mesh> mesh_;

    ResourceHandle<ShaderProgram> shader_;

    ResourceHandle<Texture> texRed;
    ResourceHandle<Texture> texGreen;
    ResourceHandle<Texture> texBlue;
    ResourceHandle<Texture> texWhite;

    ResourceHandle<ModelResource> lineModel;
    ResourceHandle<ModelResource> meshModel;

    ResourceManager resources;
    Renderer renderer;
    Scene scene;

    ResourceHandle<ModelResource> createSimpleModel(
        ResourceManager& rm,
        ResourceHandle<Mesh> mesh,
        ResourceHandle<Texture> texture,
        ResourceHandle<ShaderProgram> shader) {
        ModelResource res;

        res.meshes.push_back({
            mesh,
            texture,
            shader,
            glm::vec3(0.0f),
            glm::vec3(0.0f),
            glm::vec3(1.0f)
            });

        return rm.createModelResource(std::move(res));
    }

    void init_line(std::string name, ResourceHandle<Texture> texture, glm::vec3 rot) {
        auto lineHandle = createSimpleModel(
            resources,
            line_,
            texture,
            shader_
        );

        ModelInstance lineInstance;
        lineInstance.model = lineHandle;
        lineInstance.setEulerAngles(rot);

        scene.models.emplace(name, lineInstance);
    }
    void init_model(std::string name, ResourceHandle<Texture> texture, glm::vec3 trans, glm::vec3 scale) {
        auto modelHandle = createSimpleModel(
            resources,
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

public:
    GLint viewport[4];

    void init() {
        std::vector<Vertex> line = { Vertex{.position = {0.f,0.f,0.f}},Vertex{.position = {1.f,0.f,0.f}} };
        line_ = resources.emplaceMesh(line, GL_LINES);
        mesh_ = resources.registerMesh("../resources/assets/obj_samples/cube_triangles_vnt.obj");
        texRed = resources.emplaceTexture(glm::vec3(1.f, 0.f, 0.f));
        texGreen = resources.emplaceTexture(glm::vec3(0.f, 1.f, 0.f));
        texBlue = resources.emplaceTexture(glm::vec3(0.f, 0.f, 1.f));
        texWhite = resources.emplaceTexture(glm::vec3(1.f, 1.f, 1.f));
        shader_ = resources.emplaceShader("../resources/shaders/tex.vert", "../resources/shaders/tex.frag", false);

        glm::vec3 scale(0.2f, 0.2f, 0.2f);

        // OpenGL uses a right-handed coordinate system where
        // the positive x-axis points to the right, 
        // the positive y-axis points up, and 
        // the positive z-axis points out of the screen towards the viewer.
        auto right = glm::vec3(1.f, 0.f, 0.f);
        auto up = glm::vec3(0.f, 1.f, 0.f);
        auto front = glm::vec3(0.f, 0.f, 1.f);

        init_model("axisXModel", texRed, front, scale);
        init_line("axisXLine", texRed, glm::vec3(0.f, -90.f, 0.f));

        init_model("axisYModel", texGreen, up, scale);
        init_line("axisYLine", texGreen, glm::vec3(0.f, 0.f, 90.f));

        init_model("axisZModel", texBlue, right, scale);
        init_line("axisZLine", texBlue, glm::vec3(0.f, 0.f, 0.f));

        init_model("centerModel", texWhite, glm::vec3(0.f, 0.f, 0.f), scale * 1.5f);
    }

    void set_viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
        viewport[0] = x;
        viewport[1] = y;
        viewport[2] = width;
        viewport[3] = height;
    }

    void draw(Camera camera) {
        glClear(GL_DEPTH_BUFFER_BIT);
        GLint backup_viewport[4];
        glGetIntegerv(GL_VIEWPORT, backup_viewport);
        GLboolean backup_cullface;
        glGetBooleanv(GL_CULL_FACE, &backup_cullface);

        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        glDisable(GL_CULL_FACE);

        auto s = resources.getShader(shader_);
        s->setUniform("uV_m", glm::lookAt(-camera.Front, glm::vec3(0.f, 0.f, 0.f), camera.Up));
        s->setUniform("uP_m", glm::ortho(-1.2f, 1.2f, -1.2f, 1.2f, -0.2f, 2.2f));

        glLineWidth(1); // line widt greater than 1 is depracated

        renderer.render(&resources, &scene);

        glViewport(backup_viewport[0], backup_viewport[1], backup_viewport[2], backup_viewport[3]);
        if (backup_cullface) {
            glEnable(GL_CULL_FACE);
        }
    }
};
