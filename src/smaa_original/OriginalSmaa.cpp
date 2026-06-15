#include "smaa_original/OriginalSmaa.h"

#include <Textures/AreaTex.h>
#include <Textures/SearchTex.h>

#include <array>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

constexpr std::array<const char*, 4> kPresetDefines = {
    "SMAA_PRESET_LOW",
    "SMAA_PRESET_MEDIUM",
    "SMAA_PRESET_HIGH",
    "SMAA_PRESET_ULTRA",
};

std::string smaaPreamble(smaa_original::Preset preset, bool includeVs, bool includePs) {
    std::string preamble;
    preamble += "#define SMAA_GLSL_3 1\n";
    preamble += "#define SMAA_RT_METRICS u_rtMetrics\n";
    preamble += "#define ";
    preamble += kPresetDefines[static_cast<size_t>(preset)];
    preamble += " 1\n";
    preamble += "#define SMAA_INCLUDE_VS ";
    preamble += includeVs ? "1\n" : "0\n";
    preamble += "#define SMAA_INCLUDE_PS ";
    preamble += includePs ? "1\n" : "0\n";
    return preamble;
}

void assertFramebufferComplete() {
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("SMAA framebuffer is incomplete");
    }
}

} // namespace

namespace smaa_original {

OriginalSmaa::~OriginalSmaa() {
    release();
}

void OriginalSmaa::release() {
    if (query_ != 0) {
        glDeleteQueries(1, &query_);
        query_ = 0;
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (fbo_ != 0) {
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }

    edgeProgram_ = gl::ShaderProgram();
    blendProgram_ = gl::ShaderProgram();
    neighborhoodProgram_ = gl::ShaderProgram();
    edgesTex_ = gl::Texture2D();
    blendTex_ = gl::Texture2D();
    outputTex_ = gl::Texture2D();
    areaTex_ = gl::Texture2D();
    searchTex_ = gl::Texture2D();
    width_ = 0;
    height_ = 0;
}

void OriginalSmaa::initialize(const std::filesystem::path& shaderRoot, const std::filesystem::path& smaaRoot) {
    shaderRoot_ = shaderRoot;
    smaaRoot_ = smaaRoot;

    glGenFramebuffers(1, &fbo_);
    glGenVertexArrays(1, &vao_);
    glGenQueries(1, &query_);

    loadLookupTextures();
    compileShaders();
}

void OriginalSmaa::resize(int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }
    if (width_ == width && height_ == height && outputTex_.valid()) {
        return;
    }

    width_ = width;
    height_ = height;
    createRenderTargets();
}

void OriginalSmaa::setPreset(Preset preset) {
    if (preset_ == preset && edgeProgram_.id() != 0) {
        return;
    }

    preset_ = preset;
    if (!shaderRoot_.empty()) {
        compileShaders();
    }
}

void OriginalSmaa::execute(GLuint sourceTexture) {
    if (sourceTexture == 0 || width_ <= 0 || height_ <= 0) {
        return;
    }

    glBindVertexArray(vao_);
    timings_.edgeMs = timePass(&OriginalSmaa::edgePass, sourceTexture);
    timings_.blendMs = timePass(&OriginalSmaa::blendPass, sourceTexture);
    timings_.neighborhoodMs = timePass(&OriginalSmaa::neighborhoodPass, sourceTexture);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

DebugTextures OriginalSmaa::debugTextures() const {
    return DebugTextures{edgesTex_.id(), blendTex_.id(), outputTex_.id()};
}

void OriginalSmaa::compileShaders() {
    const auto includeDirs = std::vector<std::filesystem::path>{smaaRoot_};
    const auto vsPreamble = smaaPreamble(preset_, true, false);
    const auto psPreamble = smaaPreamble(preset_, false, true);

    edgeProgram_ = gl::ShaderProgram::fromFiles(
        shaderRoot_ / "smaa_original" / "edge.vert",
        shaderRoot_ / "smaa_original" / "edge.frag",
        includeDirs,
        vsPreamble,
        psPreamble);

    blendProgram_ = gl::ShaderProgram::fromFiles(
        shaderRoot_ / "smaa_original" / "blend.vert",
        shaderRoot_ / "smaa_original" / "blend.frag",
        includeDirs,
        vsPreamble,
        psPreamble);

    neighborhoodProgram_ = gl::ShaderProgram::fromFiles(
        shaderRoot_ / "smaa_original" / "neighborhood.vert",
        shaderRoot_ / "smaa_original" / "neighborhood.frag",
        includeDirs,
        vsPreamble,
        psPreamble);
}

void OriginalSmaa::createRenderTargets() {
    edgesTex_.createRgba8(width_, height_);
    edgesTex_.setLabel("Original SMAA edges");
    blendTex_.createRgba8(width_, height_);
    blendTex_.setLabel("Original SMAA blend weights");
    outputTex_.createRgba8(width_, height_);
    outputTex_.setLabel("Original SMAA output");
}

void OriginalSmaa::loadLookupTextures() {
    areaTex_.createRg8(AREATEX_WIDTH, AREATEX_HEIGHT, areaTexBytes);
    areaTex_.setLabel("SMAA areaTex");
    searchTex_.createR8(SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, searchTexBytes);
    searchTex_.setLabel("SMAA searchTex");
}

void OriginalSmaa::beginPass(const gl::Texture2D& target) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.id(), 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    assertFramebufferComplete();
    glViewport(0, 0, width_, height_);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    const float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    glClearBufferfv(GL_COLOR, 0, clearColor);
}

float OriginalSmaa::timePass(void (OriginalSmaa::*pass)(GLuint), GLuint sourceTexture) {
    glBeginQuery(GL_TIME_ELAPSED, query_);
    (this->*pass)(sourceTexture);
    glEndQuery(GL_TIME_ELAPSED);

    GLuint64 elapsedNs = 0;
    glGetQueryObjectui64v(query_, GL_QUERY_RESULT, &elapsedNs);
    return static_cast<float>(static_cast<double>(elapsedNs) / 1000000.0);
}

void OriginalSmaa::edgePass(GLuint sourceTexture) {
    beginPass(edgesTex_);
    edgeProgram_.use();
    edgeProgram_.setVec4("u_rtMetrics", 1.0f / static_cast<float>(width_), 1.0f / static_cast<float>(height_), static_cast<float>(width_), static_cast<float>(height_));
    edgeProgram_.setInt("u_colorTex", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sourceTexture);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void OriginalSmaa::blendPass(GLuint) {
    beginPass(blendTex_);
    blendProgram_.use();
    blendProgram_.setVec4("u_rtMetrics", 1.0f / static_cast<float>(width_), 1.0f / static_cast<float>(height_), static_cast<float>(width_), static_cast<float>(height_));
    blendProgram_.setInt("u_edgesTex", 0);
    blendProgram_.setInt("u_areaTex", 1);
    blendProgram_.setInt("u_searchTex", 2);

    edgesTex_.bind(0);
    areaTex_.bind(1);
    searchTex_.bind(2);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void OriginalSmaa::neighborhoodPass(GLuint sourceTexture) {
    beginPass(outputTex_);
    neighborhoodProgram_.use();
    neighborhoodProgram_.setVec4("u_rtMetrics", 1.0f / static_cast<float>(width_), 1.0f / static_cast<float>(height_), static_cast<float>(width_), static_cast<float>(height_));
    neighborhoodProgram_.setInt("u_colorTex", 0);
    neighborhoodProgram_.setInt("u_blendTex", 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sourceTexture);
    blendTex_.bind(1);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

const char* presetName(Preset preset) {
    switch (preset) {
    case Preset::Low:
        return "Low";
    case Preset::Medium:
        return "Medium";
    case Preset::High:
        return "High";
    case Preset::Ultra:
        return "Ultra";
    }
    return "Unknown";
}

Preset nextPreset(Preset preset) {
    const int next = (static_cast<int>(preset) + 1) % 4;
    return static_cast<Preset>(next);
}

} // namespace smaa_original
