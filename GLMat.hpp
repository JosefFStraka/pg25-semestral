#pragma once
#include <vector>
#include <cstring>
#include <stdexcept>
#include <GL/gl.h>

// --- Type system (similar to OpenCV) ---
enum GLMatType {
    GL_8UC1 = 0,
    GL_8UC3,
    GL_8UC4
};

inline int channels(GLMatType type) {
    switch (type) {
    case GL_8UC1: return 1;
    case GL_8UC3: return 3;
    case GL_8UC4: return 4;
    default: return 0;
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
    int rows = 0;
    int cols = 0;

private:
    GLMatType _type;
    std::vector<GLubyte> _data;

public:
    // --- Constructors ---
    GLMat() : _type(GL_8UC1) {}

    GLMat(int r, int c, GLMatType type)
        : rows(r), cols(c), _type(type) {
        _data.resize(rows * cols * channels(_type));
    }

    // Fill constructor (like cv::Mat(..., value))
    GLMat(int r, int c, GLMatType type, const GLubyte* value)
        : rows(r), cols(c), _type(type) {
        int ch = channels(_type);
        _data.resize(rows * cols * ch);

        for (int i = 0; i < rows * cols; ++i) {
            std::memcpy(&_data[i * ch], value, ch);
        }
    }
    // Fill constructor (like cv::Mat(..., value))
    GLMat(int r, int c, GLMatType type, const glm::vec3 value)
        : rows(r), cols(c), _type(type) {
        int ch = channels(_type);
        _data.resize(rows * cols * ch);

        for (int i = 0; i < rows * cols; ++i) {
            for (int j = 0; j < ch; j++) {
                _data[i * ch + j] = static_cast<GLubyte>(value[j] * 255.0f);
            }
        }
    }
    GLMat(int r, int c, GLMatType type, const glm::vec4 value)
        : rows(r), cols(c), _type(type) {
        int ch = channels(_type);
        _data.resize(rows * cols * ch);

        for (int i = 0; i < rows * cols; ++i) {
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
    PixelRef at(int r, int c) {
        checkBounds(r, c);
        int ch = channels(_type);
        return PixelRef{ &_data[(r * cols + c) * ch], ch };
    }

    const PixelRef at(int r, int c) const {
        checkBounds(r, c);
        int ch = channels(_type);
        return PixelRef{ const_cast<GLubyte*>(&_data[(r * cols + c) * ch]), ch };
    }

private:
    void checkBounds(int r, int c) const {
        if (r < 0 || r >= rows || c < 0 || c >= cols) {
            throw std::out_of_range("GLMat index out of range");
        }
    }
};