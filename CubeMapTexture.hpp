#include <string>
#include <vector>

#include "GLMat.hpp"
#include "NonCopyable.hpp"

#include <GL/glew.h> 
#include <glm/glm.hpp>

#include "image_io.hpp"

class CubeMapTexture : NonCopyable {
public:
    CubeMapTexture(std::vector<std::string> faces) {

        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &name_);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for (int i = 0; i < 6; i++) {
            GLMat image = imread(faces.at(i), 0);

            if (i == 0) {
                switch (image.type()) {
                case GL_8UC3: // RGB
                    glTextureStorage2D(name_, 1, GL_RGB8, image.width, image.height);
                    break;
                case GL_8UC4: // RGBA
                    glTextureStorage2D(name_, 2, GL_RGBA8, image.width, image.height);
                    break;
                case GL_16UC3: // 16-bit RGB
                    glTextureStorage2D(name_, 2, GL_RGBA8, image.width, image.height);
                    break;
                case GL_16UC4: // 16-bit RGBA
                    glTextureStorage2D(name_, 2, GL_RGBA16, image.width, image.height);
                    break;
                default:
                    throw std::runtime_error{ "unsupported number of channels or channel depth in texture" };
                }
            }

            glTextureSubImage3D(
                name_,
                0,
                0, 0, i,
                image.width,
                image.height,
                1,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                image.data()
            );
        }

        glTextureParameteri(name_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(name_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(name_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(name_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(name_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(name_, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    ~CubeMapTexture() {
        glDeleteTextures(1, &name_);
    }

    GLuint getName() {
        return name_;
    }
private:
    GLuint name_;
};
