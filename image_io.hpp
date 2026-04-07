#pragma once

#include <string>
#include "GLMat.hpp"

// Load an image from disk into a GLMat (8-bit or 16-bit, 1/3/4 channels).
// Throws std::runtime_error on failure.
GLMat imread(const std::string& path, int flag_true_if_should_flip = 1);

int imwrite(const std::string& path, GLMat image, int flip = 0);
