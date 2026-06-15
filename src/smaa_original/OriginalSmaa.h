#pragma once

#include "gl/ShaderProgram.h"
#include "gl/Texture2D.h"

#include <filesystem>
#include <string>

namespace smaa_original {

enum class Preset {
    Low = 0,
    Medium = 1,
    High = 2,
    Ultra = 3,
};

struct Timings {
    float edgeMs = 0.0f;
    float blendMs = 0.0f;
    float neighborhoodMs = 0.0f;

    float totalMs() const { return edgeMs + blendMs + neighborhoodMs; }
};

struct DebugTextures {
    GLuint edges = 0;
    GLuint blend = 0;
    GLuint output = 0;
};

class OriginalSmaa {
public:
    ~OriginalSmaa();

    void initialize(const std::filesystem::path& shaderRoot, const std::filesystem::path& smaaRoot);
    void release();
    void resize(int width, int height);
    void setPreset(Preset preset);
    void execute(GLuint sourceTexture);

    GLuint outputTexture() const { return outputTex_.id(); }
    DebugTextures debugTextures() const;
    Timings timings() const { return timings_; }
    int width() const { return width_; }
    int height() const { return height_; }
    Preset preset() const { return preset_; }

private:
    void compileShaders();
    void createRenderTargets();
    void loadLookupTextures();
    void beginPass(const gl::Texture2D& target);
    float timePass(void (OriginalSmaa::*pass)(GLuint), GLuint sourceTexture);

    void edgePass(GLuint sourceTexture);
    void blendPass(GLuint sourceTexture);
    void neighborhoodPass(GLuint sourceTexture);

    std::filesystem::path shaderRoot_;
    std::filesystem::path smaaRoot_;

    Preset preset_ = Preset::High;
    int width_ = 0;
    int height_ = 0;

    gl::ShaderProgram edgeProgram_;
    gl::ShaderProgram blendProgram_;
    gl::ShaderProgram neighborhoodProgram_;

    gl::Texture2D edgesTex_;
    gl::Texture2D blendTex_;
    gl::Texture2D outputTex_;
    gl::Texture2D areaTex_;
    gl::Texture2D searchTex_;

    GLuint fbo_ = 0;
    GLuint vao_ = 0;
    GLuint query_ = 0;

    Timings timings_;
};

const char* presetName(Preset preset);
Preset nextPreset(Preset preset);

} // namespace smaa_original
