#include "render/FullscreenView.h"

#include <algorithm>

namespace render {

FullscreenView::~FullscreenView() {
    release();
}

void FullscreenView::release() {
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    program_ = gl::ShaderProgram();
}

void FullscreenView::initialize(const std::filesystem::path& shaderRoot) {
    program_ = gl::ShaderProgram::fromFiles(
        shaderRoot / "display" / "fullscreen.vert",
        shaderRoot / "display" / "fullscreen.frag");
    glGenVertexArrays(1, &vao_);
}

void FullscreenView::draw(GLuint leftTexture, GLuint rightTexture, ViewMode mode, float split, float diffScale) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    program_.use();
    program_.setInt("u_leftTex", 0);
    program_.setInt("u_rightTex", 1);
    program_.setInt("u_viewMode", static_cast<int>(mode));
    program_.setFloat("u_split", std::clamp(split, 0.0f, 1.0f));
    program_.setFloat("u_diffScale", std::max(1.0f, diffScale));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, leftTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, rightTexture != 0 ? rightTexture : leftTexture);

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

} // namespace render
