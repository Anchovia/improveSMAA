#pragma once

#include "gl/ShaderProgram.h"

#include <filesystem>

namespace render {

enum class ViewMode {
    Final = 0,
    Split = 1,
    Difference = 2,
};

class FullscreenView {
public:
    ~FullscreenView();

    void initialize(const std::filesystem::path& shaderRoot);
    void release();
    void draw(GLuint leftTexture, GLuint rightTexture, ViewMode mode, float split, float diffScale);

private:
    gl::ShaderProgram program_;
    GLuint vao_ = 0;
};

} // namespace render
