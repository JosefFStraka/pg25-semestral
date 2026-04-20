#pragma once

#include <string>
#include <vector>
#include <memory>

#include <GL/glew.h>
#include <glm/glm.hpp> 
#include <glm/ext.hpp>

#include "assets.hpp"
#include "NonCopyable.hpp"
#include "../../aabb.hpp"

class Mesh : private NonCopyable
{
public:
    // force attribute slots in shaders for all meshes, shaders etc.
    static constexpr GLuint attribute_location_position{ 0 };
    static constexpr GLuint attribute_location_normal{ 1 };
    static constexpr GLuint attribute_location_texture_coords{ 2 };

    // No default constructor. RAII - if constructed, it will be correctly initialized
    // and can be rendered. OpenGL resources are guaranteed to be deallocated using destructor. 
    // Double-free errors are prevented by making class non-copyable (therefore 
    // double destruction of the same OpenGL buffer is prevented). 
    Mesh() = delete;

    // Simple mesh from vertices
    Mesh(std::vector<Vertex> const& vertices, GLenum primitive_type)
        : primitive_type_{ primitive_type }, vertex_count_{ static_cast<GLsizei>(vertices.size()) } {
        vao_vbo_ = std::make_shared<vao_vbo>();
        auto& vao_ = vao_vbo_->vao_;
        auto& vbo_ = vao_vbo_->vbo_;
        glCreateVertexArrays(1, &vao_);

        glVertexArrayAttribFormat(vao_, attribute_location_position, glm::vec3::length(), GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
        glVertexArrayAttribBinding(vao_, attribute_location_position, 0);
        glEnableVertexArrayAttrib(vao_, attribute_location_position);

        glVertexArrayAttribFormat(vao_, attribute_location_normal, glm::vec3::length(), GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
        glVertexArrayAttribBinding(vao_, attribute_location_normal, 0);
        glEnableVertexArrayAttrib(vao_, attribute_location_normal);

        glVertexArrayAttribFormat(vao_, attribute_location_texture_coords, glm::vec2::length(), GL_FLOAT, GL_FALSE, offsetof(Vertex, texCoords));
        glVertexArrayAttribBinding(vao_, attribute_location_texture_coords, 0);
        glEnableVertexArrayAttrib(vao_, attribute_location_texture_coords);

        glCreateBuffers(1, &vbo_);
        glNamedBufferData(vbo_, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glVertexArrayVertexBuffer(vao_, 0, vbo_, 0, sizeof(Vertex));
    }

    // Mesh with indirect vertex addressing. Needs compiled shader for attributes setup. 
    Mesh(std::vector<Vertex> const& vertices, std::vector<GLuint> const& indices, GLenum primitive_type)
        : Mesh{ vertices, primitive_type } {
        index_count_ = static_cast<GLsizei>(indices.size());
        glCreateBuffers(1, &ebo_);
        glNamedBufferData(ebo_, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        glVertexArrayElementBuffer(vao_vbo_->vao_, ebo_);
    }

    // Reuse 
    Mesh(Mesh&& other, std::vector<GLuint> const& indices)
        : primitive_type_{ other.primitive_type_ }
        , vertex_count_{ other.vertex_count_ }
        , index_count_{ other.index_count_ }
        , vao_vbo_{ other.vao_vbo_ }
        , ebo_{ other.ebo_ } {
        index_count_ = static_cast<GLsizei>(indices.size());
        glCreateBuffers(1, &ebo_);
        glNamedBufferData(ebo_, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        glVertexArrayElementBuffer(vao_vbo_->vao_, ebo_);
    }

    Mesh(Mesh&& other) noexcept
        : primitive_type_{ other.primitive_type_ }
        , vertex_count_{ other.vertex_count_ }
        , index_count_{ other.index_count_ }
        , vao_vbo_{ other.vao_vbo_ }
        , ebo_{ other.ebo_ } {
        // Nullify source so its destructor won't delete our GL objects
        vao_vbo_ = nullptr;
        other.ebo_ = 0;
        other.vertex_count_ = 0;
        other.index_count_ = 0;
    }

    void bind() {
        glBindVertexArray(vao_vbo_->vao_);
    }

    bool hasEbo() {
        return ebo_ != 0;
    }

    GLenum  getPrimitiveType() { return primitive_type_; }
    GLsizei getVertexCount() { return vertex_count_; }
    GLsizei getIndexCount() { return index_count_; }

    ~Mesh() {
        glDeleteBuffers(1, &ebo_);
    };

    AABB aabb_;
private:
    struct vao_vbo {
        GLuint vao_{ 0 };
        GLuint vbo_{ 0 };

        ~vao_vbo() {
            glDeleteBuffers(1, &vbo_);
            glDeleteVertexArrays(1, &vao_);
        }
    };

    // safe defaults
    GLenum  primitive_type_{ GL_POINTS };
    GLsizei vertex_count_{ 0 };
    GLsizei index_count_{ 0 };

    // OpenGL buffer IDs
    // ID = 0 is reserved (i.e. uninitalized)

    std::shared_ptr<vao_vbo> vao_vbo_;
    GLuint ebo_{ 0 };
};



