
#include "Model.hpp"
#include "camera.hpp"
#include "meshgen.hpp"

class AxisDisplay {
    std::shared_ptr<Mesh> line_;
    std::shared_ptr<Mesh> mesh_;
    std::shared_ptr<ShaderProgram> shader_;

    Model axisXLine;
    Model axisXModel;
    Model axisYLine;
    Model axisYModel;
    Model axisZLine;
    Model axisZModel;
    Model centerModel;

    void init_line(Model& model, glm::vec3 rot) {
        model.addMesh(line_, shader_);
        model.setEulerAngles(rot);
    }
    void init_model(Model& model, glm::vec3 trans, glm::vec3 scale) {
        model.addMesh(mesh_, shader_);
        model.setPosition(trans);
        model.setScale(scale);
    }

public:
    GLint viewport[4];

    void init(std::shared_ptr<Mesh> line, std::shared_ptr<Mesh> mesh, std::shared_ptr<ShaderProgram> shader) {
        line_ = line;
        mesh_ = mesh;
        shader_ = shader;

        glm::vec3 origin_offset(-0.5f, -0.5f, -0.5f);
        glm::vec3 scale(0.2f, 0.2f, 0.2f);
        origin_offset *= scale;

        // OpenGL uses a right-handed coordinate system where
        // the positive x-axis points to the right, 
        // the positive y-axis points up, and 
        // the positive z-axis points out of the screen towards the viewer.

        auto right = glm::vec3(1.f, 0.f, 0.f);
        auto up = glm::vec3(0.f, 1.f, 0.f);
        auto front = glm::vec3(0.f, 0.f, 1.f);

        init_model(axisXModel, right, scale);
        init_line(axisXLine, glm::vec3(0.f, 0.f, 0.f));

        init_model(axisYModel, up, scale);
        init_line(axisYLine, glm::vec3(0.f, 0.f, 90.f));

        init_line(axisZLine, glm::vec3(0.f, -90.f, 0.f));
        init_model(axisZModel, front, scale);

        init_model(centerModel, glm::vec3(0.f, 0.f, 0.f), scale * 1.5f);
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
        shader_->setUniform("uV_m", glm::lookAt(-camera.Front, glm::vec3(0.f, 0.f, 0.f), camera.Up));
        shader_->setUniform("uP_m", glm::ortho(-1.2f, 1.2f, -1.2f, 1.2f, -0.2f, 2.2f));
        shader_->use();

        glLineWidth(1); // line widt greater than 1 is depracated

        shader_->setUniform("ucolor", glm::vec4(255.f, 0.f, 0.f, 1.f));
        axisXLine.draw();
        axisXModel.draw();
        shader_->setUniform("ucolor", glm::vec4(0.f, 255.f, 0.f, 1.f));
        axisYLine.draw();
        axisYModel.draw();
        shader_->setUniform("ucolor", glm::vec4(0.f, 0.f, 255.f, 1.f));
        axisZLine.draw();
        axisZModel.draw();
        shader_->setUniform("ucolor", glm::vec4(255.f, 255.f, 255.f, 1.f));
        centerModel.draw();

        glViewport(backup_viewport[0], backup_viewport[1], backup_viewport[2], backup_viewport[3]);
        if (backup_cullface) {
            glEnable(GL_CULL_FACE);
        }
    }
};
