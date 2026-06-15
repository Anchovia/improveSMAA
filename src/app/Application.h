#pragma once

#include "render/FullscreenView.h"
#include "render/ImageSource.h"
#include "render/SceneLibrary.h"
#include "render/SceneRenderer.h"
#include "smaa_original/OriginalSmaa.h"
#include "smaa_variant/SmaaVariant.h"

#include <GLFW/glfw3.h>

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace app {

class Application {
public:
    enum class AaMode {
        None = 0,
        OriginalSmaa = 1,
        AdaptiveSmaa = 2,
        AdaptiveTscmaaSmaa = 3,
    };

    Application();
    ~Application();

    int run(int argc, char** argv);

private:
    enum class InputMode {
        Image = 0,
        Scene = 1,
    };

    enum class DisplayMode {
        Final = 0,
        Split = 1,
        Difference = 2,
        Edges = 3,
        BlendWeights = 4,
    };

    enum class SourceType {
        GeneratedPattern = 0,
        ImageFile = 1,
        Scene = 2,
    };

    struct SourceEntry {
        SourceType type = SourceType::GeneratedPattern;
        std::string label;
        std::filesystem::path path;
        int sceneIndex = -1;
    };

    void initialize(int argc, char** argv);
    void shutdown();
    void mainLoop();
    void renderFrame();
    void drawUi();
    void drawSourcePanel();
    void drawAaPanel();
    void drawInspectPanel();
    void drawStatsPanel();
    void drawAdvancedPanel();
    void rebuildSourceEntries();
    void requestLoadSelectedSource();
    void processPendingSourceLoad();
    void loadSelectedSource();
    bool loadImageFromUi();
    bool loadImagePath(const std::filesystem::path& path);
    void reloadSceneManifest();
    bool loadSelectedScene();
    void resetSceneCamera(float exposure);
    void updateSceneCameraInput();
    void resizeSmaaTargets(int sourceWidth, int sourceHeight);
    void updateSmaa(GLuint sourceTexture, int sourceWidth, int sourceHeight);
    smaa_original::DebugTextures selectedDebugTextures() const;
    smaa_original::Timings selectedTimings() const;

    std::filesystem::path rootDir_;
    std::filesystem::path shaderRoot_;
    std::filesystem::path smaaRoot_;

    GLFWwindow* window_ = nullptr;
    bool glfwInitialized_ = false;
    bool imguiInitialized_ = false;

    render::ImageSource image_;
    render::SceneLibrary sceneLibrary_;
    render::SceneRenderer sceneRenderer_;
    render::SceneCamera sceneCamera_;
    render::FullscreenView fullscreenView_;
    smaa_original::OriginalSmaa originalSmaa_;
    smaa_variant::SmaaVariant adaptiveSmaa_;
    smaa_variant::SmaaVariant adaptiveTscmaaSmaa_;

    InputMode inputMode_ = InputMode::Image;
    AaMode aaMode_ = AaMode::OriginalSmaa;
    DisplayMode displayMode_ = DisplayMode::Final;
    smaa_original::Preset preset_ = smaa_original::Preset::High;

    float split_ = 0.5f;
    float diffScale_ = 8.0f;
    int sceneRenderWidth_ = 1280;
    int sceneRenderHeight_ = 720;
    std::vector<SourceEntry> sourceEntries_;
    int selectedSource_ = 0;
    int loadedSource_ = 0;
    bool pendingSourceLoad_ = false;
    int selectedScene_ = -1;
    bool sceneMouseOrbitEnabled_ = false;
    bool sceneMiddleMouseWasDown_ = false;
    bool rotatingSceneCamera_ = false;
    double lastSceneCameraTime_ = 0.0;
    double lastSceneCursorX_ = 0.0;
    double lastSceneCursorY_ = 0.0;

    std::array<char, 1024> imagePath_{};
    std::array<char, 1024> sceneManifestPath_{};
    std::string statusText_;
    std::string errorText_;
};

} // namespace app
