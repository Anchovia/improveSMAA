#include "gl/Texture2D.h"

#include <utility>

namespace gl {

Texture2D::~Texture2D() {
    reset();
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : texture_(std::exchange(other.texture_, 0)),
      width_(std::exchange(other.width_, 0)),
      height_(std::exchange(other.height_, 0)) {}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this != &other) {
        reset();
        texture_ = std::exchange(other.texture_, 0);
        width_ = std::exchange(other.width_, 0);
        height_ = std::exchange(other.height_, 0);
    }
    return *this;
}

void Texture2D::createRgba8(int width, int height, const std::uint8_t* pixels) {
    ensureCreated();
    width_ = width;
    height_ = height;
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    setLinearClamp();
}

void Texture2D::createRg8(int width, int height, const std::uint8_t* pixels) {
    ensureCreated();
    width_ = width;
    height_ = height;
    glBindTexture(GL_TEXTURE_2D, texture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, width_, height_, 0, GL_RG, GL_UNSIGNED_BYTE, pixels);
    setLinearClamp();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void Texture2D::createR8(int width, int height, const std::uint8_t* pixels) {
    ensureCreated();
    width_ = width;
    height_ = height;
    glBindTexture(GL_TEXTURE_2D, texture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width_, height_, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
    setLinearClamp();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void Texture2D::bind(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture_);
}

void Texture2D::setLinearClamp() const {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void Texture2D::setNearestClamp() const {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void Texture2D::setLabel(std::string_view label) const {
    (void)label;
}

void Texture2D::ensureCreated() {
    if (texture_ == 0) {
        glGenTextures(1, &texture_);
    }
}

void Texture2D::reset() {
    if (texture_ != 0) {
        glDeleteTextures(1, &texture_);
        texture_ = 0;
    }
    width_ = 0;
    height_ = 0;
}

} // namespace gl
