#pragma once

#include "gl/ShaderProgram.h"
#include "gl/Texture2D.h"
#include "smaa_original/OriginalSmaa.h"

#include <filesystem>
#include <string>

namespace smaa_variant {

class SmaaVariant {
public:
    SmaaVariant() = default;
    explicit SmaaVariant(std::string displayName, std::string shaderDirectory);
    ~SmaaVariant();

    SmaaVariant(const SmaaVariant&) = delete;
    SmaaVariant& operator=(const SmaaVariant&) = delete;

    void initialize(const std::filesystem::path& shaderRoot, const std::filesystem::path& smaaRoot);
    void release();
    void resize(int width, int height);
    void setPreset(smaa_original::Preset preset);
    void execute(GLuint sourceTexture);

    GLuint outputTexture() const { return outputTex_.id(); }
    smaa_original::DebugTextures debugTextures() const;
    smaa_original::Timings timings() const { return timings_; }
    const std::string& displayName() const { return displayName_; }
    smaa_original::Preset preset() const { return preset_; }

private:
    void compileShaders();
    void createRenderTargets();
    void loadLookupTextures();
    void beginPass(const gl::Texture2D& target);
    float timePass(void (SmaaVariant::*pass)(GLuint), GLuint sourceTexture);

    void edgePass(GLuint sourceTexture);
    void blendPass(GLuint sourceTexture);
    void neighborhoodPass(GLuint sourceTexture);

    std::string displayName_ = "SMAA Variant";
    std::string shaderDirectory_;
    std::filesystem::path shaderRoot_;
    std::filesystem::path smaaRoot_;

    smaa_original::Preset preset_ = smaa_original::Preset::High;
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

    smaa_original::Timings timings_;
};

} // namespace smaa_variant
