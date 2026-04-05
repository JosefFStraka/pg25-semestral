#include "image_io.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <cstring>
#include <stdexcept>


GLMat imread(const std::string& path, int flag_true_if_should_flip) {
    int width, height, channels;

    bool is_16bit = stbi_is_16_bit(path.c_str());

    stbi_set_flip_vertically_on_load(flag_true_if_should_flip);

    if (!is_16bit) {
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
    } else {
        unsigned short* data = stbi_load_16(path.c_str(), &width, &height, &channels, 0);

        if (!data) {
            throw std::runtime_error("Failed to load image: " + path);
        }

        GLMatType type;
        switch (channels) {
        case 1: type = GL_16UC1; break;
        case 3: type = GL_16UC3; break;
        case 4: type = GL_16UC4; break;
        default:
            stbi_image_free(data);
            throw std::runtime_error("Unsupported channel count");
        }

        GLMat img(height, width, type);

        std::memcpy(img.data(), data, width * height * channels);

        stbi_image_free(data);
        return img;
    }
}

int imwrite(const std::string& path, GLMat image, int flip) {
    int ch = channels(image.type());
    int stride_in_bytes = channelSize(image.type()) * ch  * image.width;

    stbi_flip_vertically_on_write(flip);

    int result = stbi_write_png(
        path.c_str(),
        image.width,
        image.height,
        ch,
        reinterpret_cast<const void*>(image.data()),
        stride_in_bytes
    );

    return result;
}
