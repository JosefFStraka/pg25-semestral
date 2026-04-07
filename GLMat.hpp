#pragma once
#include <vector>
#include <cstring>
#include <stdexcept>
#include <GL/glew.h>
#include <glm/glm.hpp>
// --- Type system (similar to OpenCV) ---
enum GLMatType {
    GL_8UC1 = 0,
    GL_8UC3,
    GL_8UC4,
    GL_16UC1,
    GL_16UC3,
    GL_16UC4
};

inline int channels(GLMatType type) {
    switch (type) {
    case GL_8UC1: case GL_16UC1: return 1;
    case GL_8UC3: case GL_16UC3: return 3;
    case GL_8UC4: case GL_16UC4: return 4;
    default: return 0;
    }
}

inline int channelSize(GLMatType type) {
    switch (type) {
    case GL_8UC1: case GL_8UC3: case GL_8UC4: return sizeof(GLubyte); break;
    case GL_16UC1: case GL_16UC3: case GL_16UC4: return sizeof(GLushort); break;
    default: return -1; break;
    }
}

inline int elemSize(GLMatType type) {
    return channels(type) * sizeof(GLubyte);
}

struct PixelRef {
    GLubyte* ptr;
    int ch;

    // Assign from array (e.g. GLubyte[3])
    PixelRef& operator=(const GLubyte* value) {
        std::memcpy(ptr, value, ch);
        return *this;
    }

    // Assign from initializer list (e.g. {255,255,255})
    PixelRef& operator=(std::initializer_list<GLubyte> list) {
        int i = 0;
        for (auto v : list) {
            if (i >= ch) break;
            ptr[i++] = v;
        }
        return *this;
    }

    // Optional: access channels
    GLubyte& operator[](int i) {
        return ptr[i];
    }

    const GLubyte& operator[](int i) const {
        return ptr[i];
    }
};

// --- GLMat class ---
class GLMat {
public:
    int height = 0;
    int width = 0;

private:
    GLMatType _type;
    std::vector<GLubyte> _data;

public:
    // --- Constructors ---
    GLMat() : _type(GL_8UC1) {}

    GLMat(int h, int w, GLMatType type)
        : height(h), width(w), _type(type) {
        _data.resize(height * width * channels(_type));
    }

    // Fill constructor (like cv::Mat(..., value))
    GLMat(int h, int w, GLMatType type, const GLubyte* value)
        : height(h), width(w), _type(type) {
        int ch = channels(_type);
        _data.resize(height * width * ch);

        for (int i = 0; i < height * width; ++i) {
            std::memcpy(&_data[i * ch], value, ch);
        }
    }
    // Fill constructor (like cv::Mat(..., value))
    GLMat(int h, int w, GLMatType type, const glm::vec3 value)
        : height(h), width(w), _type(type) {
        int ch = channels(_type);
        _data.resize(height * width * ch);

        for (int i = 0; i < height * width; ++i) {
            for (int j = 0; j < ch; j++) {
                _data[i * ch + j] = static_cast<GLubyte>(value[j] * 255.0f);
            }
        }
    }
    GLMat(int h, int w, GLMatType type, const glm::vec4 value)
        : height(h), width(w), _type(type) {
        int ch = channels(_type);
        _data.resize(height * width * ch);

        for (int i = 0; i < height * width; ++i) {
            for (int j = 0; j < ch; j++) {
                _data[i * ch + j] = static_cast<GLubyte>(value[j] * 255.0f);
            }
        }
    }

    // --- Core API ---
    bool empty() const {
        return _data.empty();
    }

    GLMatType type() const {
        return _type;
    }

    GLubyte* data() {
        return _data.data();
    }

    const GLubyte* data() const {
        return _data.data();
    }

    // --- Element access ---
    PixelRef at(int h, int w) {
        checkBounds(h, w);
        int ch = channels(_type);
        return PixelRef{ &_data[(h * width + w) * ch], ch };
    }

    const PixelRef at(int h, int w) const {
        checkBounds(h, w);
        int ch = channels(_type);
        return PixelRef{ const_cast<GLubyte*>(&_data[(h * width + w) * ch]), ch };
    }

private:
    void checkBounds(int h, int w) const {
        if (h < 0 || h >= height || w < 0 || w >= width) {
            throw std::out_of_range("GLMat index out of range");
        }
    }
};