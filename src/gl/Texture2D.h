#pragma once

#include "gl/Gl.h"

#include <cstdint>
#include <string_view>

namespace gl {

class Texture2D {
public:
    Texture2D() = default;
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    void createRgba8(int width, int height, const std::uint8_t* pixels = nullptr);
    void createRg8(int width, int height, const std::uint8_t* pixels);
    void createR8(int width, int height, const std::uint8_t* pixels);

    void bind(GLuint unit) const;
    void setLinearClamp() const;
    void setLinearRepeat() const;
    void setNearestClamp() const;
    void setLabel(std::string_view label) const;

    GLuint id() const { return texture_; }
    int width() const { return width_; }
    int height() const { return height_; }
    bool valid() const { return texture_ != 0; }

private:
    void ensureCreated();
    void reset();

    GLuint texture_ = 0;
    int width_ = 0;
    int height_ = 0;
};

} // namespace gl
