#include "Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <string>
#include <stdexcept>

// Mimic OpenCV flag
enum GLImreadFlags {
    GL_IMREAD_UNCHANGED = -1
};

GLMat imread(const std::string& path, int flag = GL_IMREAD_UNCHANGED)
{
    int width, height, channels;

    // Load without forcing channels (UNCHANGED behavior)
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (!data) {
        throw std::runtime_error("Failed to load image: " + path);
    }

    GLMatType type;
    switch (channels) {
        case 1: type = GL_8UC1; break;
        case 3: type = GL_8UC3; break;
        case 4: type = GL_8UC4; break;
        default:
            stbi_image_free(data);
            throw std::runtime_error("Unsupported channel count");
    }

    GLMat img(height, width, type);

    std::memcpy(img.data(), data, width * height * channels);

    stbi_image_free(data);
    return img;
}

void flip(GLMat const& src, GLMat& dst, int flipCode)
{
    if (src.empty()) {
        throw std::runtime_error("flip: source is empty");
    }

    dst = GLMat(src.rows, src.cols, src.type());

    int ch = channels(src.type());

    for (int r = 0; r < src.rows; ++r) {
        for (int c = 0; c < src.cols; ++c) {

            int rr = r;
            int cc = c;

            if (flipCode == 0 || flipCode == -1)
                rr = src.rows - 1 - r;

            if (flipCode == 1 || flipCode == -1)
                cc = src.cols - 1 - c;

            const GLubyte* src_px = src.at(r, c).ptr;
            GLubyte* dst_px = dst.at(rr, cc).ptr;

            std::memcpy(dst_px, src_px, ch);
        }
    }
}

GLuint Texture::gen_ckboard(void) {
    if (glIsTexture(ckboard_) != GL_TRUE) { // default checker-board texture yet not valid texture
        glCreateTextures(GL_TEXTURE_2D, 1, &ckboard_);

        GLubyte black[3] = {1, 2, 3};
        GLubyte white[3] = {255, 255, 255};
        GLMat ckb = GLMat(2, 2, GLMatType::GL_8UC3, black);
        ckb.at(0, 0) = white;
        ckb.at(1, 1) = white;

        glTextureStorage2D(ckboard_, 1, GL_RGB8, ckb.cols, ckb.rows);
        glTextureSubImage2D(ckboard_, 0, 0, 0, ckb.cols, ckb.rows, GL_BGR, GL_UNSIGNED_BYTE, ckb.data());
        glTextureParameteri(ckboard_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(ckboard_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(ckboard_, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(ckboard_, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    return ckboard_;
}

GLMat Texture::load_image(const std::filesystem::path& path) {
    GLMat image = imread(path.string()); // Read with (potential) alpha, do not rotate by EXIF.

    // check! cv::imread does NOT throw exception, if the image is not found.
    if (image.empty()) {
        throw std::runtime_error{ std::string("no texture in file: ").append(path.string()) };
    }
    return image;
}

Texture::Texture(const std::filesystem::path & path, Interpolation interpolation) : Texture{ load_image(path), interpolation } {}

Texture::Texture(const glm::vec3 & vec) : Texture{ GLMat{1, 1, GL_8UC3, {vec.b, vec.g, vec.r}}, Interpolation::nearest } {}

Texture::Texture(const glm::vec4 & vec) : Texture{ GLMat{1, 1, GL_8UC4, {vec.b, vec.g, vec.r, vec.a}}, Interpolation::nearest } {}

Texture::Texture(GLMat const& image, Interpolation interpolation)
{
    if (ckboard_ == 0) {
        ckboard_ = gen_ckboard();
    }

    if (image.empty()) {
        throw std::runtime_error{ "the input image is empty" };
    }

    GLMat flipped;
    flip(image, flipped, 0);  // OpenGL vs. Window coordinates...

    glCreateTextures(GL_TEXTURE_2D, 1, &name_);

    switch (flipped.type()) {
        case GL_8UC1: // single channel image - greyscale
        // upload only one channel
        glTextureStorage2D(name_, 1, GL_R8, flipped.cols, flipped.rows);
        glTextureSubImage2D(name_, 0, 0, 0, flipped.cols, flipped.rows, GL_RED, GL_UNSIGNED_BYTE, flipped.data());
        // use data also for other channels
        glTextureParameteri(name_, GL_TEXTURE_SWIZZLE_G, GL_RED);
        glTextureParameteri(name_, GL_TEXTURE_SWIZZLE_B, GL_RED);
        break;
    case GL_8UC3:  // RGB
        // upload only one channel
        glTextureStorage2D(name_, 1, GL_RGB8, flipped.cols, flipped.rows);
        glTextureSubImage2D(name_, 0, 0, 0, flipped.cols, flipped.rows,  GL_BGR, GL_UNSIGNED_BYTE, flipped.data());
        break;
    case GL_8UC4:  // RGBA
        // upload only one channel
        glTextureStorage2D(name_, 1, GL_RGBA8, flipped.cols, flipped.rows);
        glTextureSubImage2D(name_, 0, 0, 0, flipped.cols, flipped.rows,  GL_BGR, GL_UNSIGNED_BYTE, flipped.data());
        break;
    default:
        throw std::runtime_error{ "unsupported number of channels or channel depth in texture" };
    }

    set_interpolation(interpolation);

    // Configures the way the texture repeats
    //TODO glTextureParameteri(...)
    glTextureParameteri(name_, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(name_, GL_TEXTURE_WRAP_R, GL_REPEAT);
}

Texture::~Texture() {
    glDeleteTextures(1, &name_);
}

GLuint Texture::get_name() const {
    return name_;
}

void Texture::bind(void) {
    glBindTextureUnit(0, name_); // bind to some texturing unit, e.g. 0
}

void Texture::set_interpolation(Interpolation interpolation) {
    // Select texture filering method 
    switch (interpolation) {
    case Interpolation::nearest:
        // nearest neighbor - ugly & fast 
        glTextureParameteri(name_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(name_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        break;
    case Interpolation::linear:
        // bilinear - nicer & slower
        glTextureParameteri(name_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(name_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;
    case Interpolation::linear_mipmap_linear:
        // Trilinear: MIPMAP filtering + automatic MIPMAP generation - nicest, needs more memory. Notice: MIPMAP is only for image minifying.
        glTextureParameteri(name_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);               // bilinear magnifying
        glTextureParameteri(name_, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // trilinear minifying
        glGenerateTextureMipmap(name_);  // Generate mipmaps now.
        break;
    }
}

int Texture::get_height(void) {
    int tex_height = 0;
    int basemiplevel = 0; // base image
    glGetTextureLevelParameteriv(name_, basemiplevel, GL_TEXTURE_HEIGHT, &tex_height);

    return tex_height;
}

int Texture::get_width(void) {
    int tex_width = 0;
    int basemiplevel = 0; // base image
    glGetTextureLevelParameteriv(name_, basemiplevel, GL_TEXTURE_WIDTH, &tex_width);

    return tex_width;
}

void Texture::replace_image(const GLMat& image) {
    // immutable texture format used: only content can be changed (size and data format MUST match)

    // check size
    if ((image.rows != get_height() ) || (image.cols != get_width()))
        throw std::runtime_error("improper image replacement size");

    // check channels and format
    int tex_format = 0;
    int basemiplevel = 0; // base image
    glGetTextureLevelParameteriv(name_, basemiplevel, GL_TEXTURE_INTERNAL_FORMAT, &tex_format);

    switch (image.type()) {
    case GL_8UC1: // single channel image - greyscale
        if (tex_format != GL_R8)
            throw std::runtime_error("improper image replacement channel data, GL_R8 was the original");
        glTextureSubImage2D(name_, 0, 0, 0, image.cols, image.rows, GL_RED, GL_UNSIGNED_BYTE, image.data());
        break;
    case GL_8UC3:  // RGB
        if (tex_format != GL_RGB8)
            throw std::runtime_error("improper image replacement channel data, GL_RGB8 was the original");
        glTextureSubImage2D(name_, 0, 0, 0, image.cols, image.rows, GL_BGR, GL_UNSIGNED_BYTE, image.data());
        break;
    case GL_8UC4:  // RGBA
        if (tex_format != GL_RGBA8)
            throw std::runtime_error("improper image replacement channel data, GL_RGBA8 was the original");
        glTextureSubImage2D(name_, 0, 0, 0, image.cols, image.rows, GL_BGRA, GL_UNSIGNED_BYTE, image.data());
        break;
    default:
        throw std::runtime_error{ "unsupported number of channels or channel depth in texture" };
    }
}
