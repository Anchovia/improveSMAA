#pragma once

#include "gl/ShaderProgram.h"
#include "gl/Texture2D.h"
#include "render/SceneLibrary.h"

#include <filesystem>
#include <string>
#include <vector>

namespace render {

struct SceneCamera {
    float yaw = 0.75f;
    float pitch = 0.35f;
    float distance = 2.4f;
    float exposure = 1.0f;
};

class SceneRenderer {
public:
    ~SceneRenderer();

    void initialize(const std::filesystem::path& shaderRoot);
    void release();
    bool loadObjScene(
        const SceneEntry& entry,
        const std::filesystem::path& manifestPath,
        std::string& error);
    void resize(int width, int height);
    void render(const SceneCamera& camera);

    GLuint colorTexture() const { return colorTex_.id(); }
    int width() const { return width_; }
    int height() const { return height_; }
    bool hasScene() const { return vertexCount_ > 0; }
    const std::string& label() const { return label_; }

private:
    struct Bounds {
        float minX = 0.0f;
        float minY = 0.0f;
        float minZ = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
        float maxZ = 0.0f;
    };

    void createFramebuffer();
    void uploadVertices(const std::vector<float>& vertices);

    gl::ShaderProgram program_;
    gl::Texture2D colorTex_;

    GLuint fbo_ = 0;
    GLuint depthRbo_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;

    int width_ = 1280;
    int height_ = 720;
    GLsizei vertexCount_ = 0;
    Bounds bounds_;
    std::string label_ = "No scene";
};

} // namespace render
