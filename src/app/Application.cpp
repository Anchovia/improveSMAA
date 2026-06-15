#include "app/Application.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {

void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW error " << error << ": " << description << '\n';
}

template <typename T, size_t N>
void setBuffer(std::array<T, N>& buffer, const std::string& text) {
    std::fill(buffer.begin(), buffer.end(), T{});
    const auto count = std::min(text.size(), N - 1);
    std::memcpy(buffer.data(), text.data(), count);
}

const char* aaModeName(app::Application::AaMode mode) {
    switch (mode) {
    case app::Application::AaMode::None:
        return "No AA";
    case app::Application::AaMode::OriginalSmaa:
        return "Original SMAA";
    case app::Application::AaMode::AdaptiveSmaa:
        return "Adaptive SMAA";
    case app::Application::AaMode::AdaptiveTscmaaSmaa:
        return "Adaptive + TSCMAA SMAA";
    }
    return "Unknown";
}

GLProc loadGlfwOpenGLProc(const char* name) {
    return glfwGetProcAddress(name);
}

std::string lowerCopy(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

} // namespace

namespace app {

Application::Application()
    : adaptiveSmaa_("Adaptive SMAA", "smaa_adaptive"),
      adaptiveTscmaaSmaa_("Adaptive + TSCMAA SMAA", "smaa_adaptive_tscmaa") {}

Application::~Application() {
    shutdown();
}

int Application::run(int argc, char** argv) {
    try {
        initialize(argc, argv);
        mainLoop();
        shutdown();
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        shutdown();
        return 1;
    }
}

void Application::initialize(int argc, char** argv) {
    rootDir_ = std::filesystem::path(IMPROVESMAA_ROOT_DIR);
    shaderRoot_ = rootDir_ / "shaders";
    smaaRoot_ = rootDir_ / "external" / "iryoku_smaa";

    setBuffer(sceneManifestPath_, (rootDir_ / "assets" / "scenes" / "scenes.json").string());

    glfwSetErrorCallback(glfwErrorCallback);
    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    glfwInitialized_ = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(1600, 900, "improveSMAA", nullptr, nullptr);
    if (window_ == nullptr) {
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    if (!gl::loadOpenGL(loadGlfwOpenGLProc)) {
        throw std::runtime_error("Failed to load OpenGL functions");
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    imguiInitialized_ = true;

    image_.createDefaultPattern(1280, 720);
    if (argc > 1) {
        setBuffer(imagePath_, argv[1]);
        loadImageFromUi();
    }

    fullscreenView_.initialize(shaderRoot_);
    sceneRenderer_.initialize(shaderRoot_);
    originalSmaa_.initialize(shaderRoot_, smaaRoot_);
    adaptiveSmaa_.initialize(shaderRoot_, smaaRoot_);
    adaptiveTscmaaSmaa_.initialize(shaderRoot_, smaaRoot_);
    resizeSmaaTargets(image_.texture().width(), image_.texture().height());
    reloadSceneManifest();
}

void Application::shutdown() {
    if (window_ == nullptr && !glfwInitialized_) {
        return;
    }

    originalSmaa_.release();
    adaptiveSmaa_.release();
    adaptiveTscmaaSmaa_.release();
    sceneRenderer_.release();
    fullscreenView_.release();
    image_.release();

    if (imguiInitialized_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        imguiInitialized_ = false;
    }

    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    if (glfwInitialized_) {
        glfwTerminate();
        glfwInitialized_ = false;
    }
}

void Application::mainLoop() {
    while (glfwWindowShouldClose(window_) == GLFW_FALSE) {
        glfwPollEvents();
        renderFrame();
        glfwSwapBuffers(window_);
    }
}

void Application::renderFrame() {
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window_, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    GLuint sourceTexture = image_.texture().id();
    int sourceWidth = image_.texture().width();
    int sourceHeight = image_.texture().height();

    if (inputMode_ == InputMode::Scene && sceneRenderer_.hasScene()) {
        sceneRenderer_.resize(sceneRenderWidth_, sceneRenderHeight_);
        sceneRenderer_.render(sceneCamera_);
        sourceTexture = sceneRenderer_.colorTexture();
        sourceWidth = sceneRenderer_.width();
        sourceHeight = sceneRenderer_.height();
    }

    updateSmaa(sourceTexture, sourceWidth, sourceHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    GLuint leftTexture = sourceTexture;
    GLuint rightTexture = leftTexture;
    auto viewMode = render::ViewMode::Final;

    const auto debug = selectedDebugTextures();
    if (aaMode_ != AaMode::None && debug.output != 0) {
        rightTexture = debug.output;
    }

    switch (displayMode_) {
    case DisplayMode::Final:
        viewMode = render::ViewMode::Final;
        break;
    case DisplayMode::Split:
        viewMode = render::ViewMode::Split;
        break;
    case DisplayMode::Difference:
        viewMode = render::ViewMode::Difference;
        break;
    case DisplayMode::Edges:
        rightTexture = debug.edges != 0 ? debug.edges : rightTexture;
        viewMode = render::ViewMode::Final;
        break;
    case DisplayMode::BlendWeights:
        rightTexture = debug.blend != 0 ? debug.blend : rightTexture;
        viewMode = render::ViewMode::Final;
        break;
    }

    glClearColor(0.02f, 0.022f, 0.026f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    fullscreenView_.draw(leftTexture, rightTexture, viewMode, split_, diffScale_);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    drawUi();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Application::drawUi() {
    ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(430.0f, 620.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("improveSMAA");

    drawInputPanel();
    ImGui::Separator();
    drawComparisonPanel();
    ImGui::Separator();
    drawScenePanel();

    if (!statusText_.empty()) {
        ImGui::Separator();
        ImGui::TextWrapped("%s", statusText_.c_str());
    }
    if (!errorText_.empty()) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.28f, 1.0f), "%s", errorText_.c_str());
    }

    ImGui::End();
}

void Application::drawInputPanel() {
    ImGui::TextUnformatted("Input");

    int inputMode = static_cast<int>(inputMode_);
    const char* inputLabels[] = {"Image", "Scene"};
    if (ImGui::Combo("Input mode", &inputMode, inputLabels, IM_ARRAYSIZE(inputLabels))) {
        inputMode_ = static_cast<InputMode>(inputMode);
    }

    if (inputMode_ == InputMode::Scene && sceneRenderer_.hasScene()) {
        ImGui::Text("Current: %s", sceneRenderer_.label().c_str());
    } else {
        ImGui::Text("Current: %s", image_.label().c_str());
    }

    ImGui::InputText("Image path", imagePath_.data(), imagePath_.size());
    if (ImGui::Button("Load image")) {
        loadImageFromUi();
    }
    ImGui::SameLine();
    if (ImGui::Button("Generated pattern")) {
        image_.createDefaultPattern(1280, 720);
        resizeSmaaTargets(image_.texture().width(), image_.texture().height());
        statusText_ = "Using generated test pattern.";
        errorText_.clear();
        inputMode_ = InputMode::Image;
    }
}

void Application::drawComparisonPanel() {
    ImGui::TextUnformatted("Comparison");

    int aaMode = static_cast<int>(aaMode_);
    const char* aaLabels[] = {"No AA", "Original SMAA", "Adaptive SMAA", "Adaptive + TSCMAA SMAA"};
    if (ImGui::Combo("AA mode", &aaMode, aaLabels, IM_ARRAYSIZE(aaLabels))) {
        aaMode_ = static_cast<AaMode>(aaMode);
    }

    int preset = static_cast<int>(preset_);
    const char* presetLabels[] = {"Low", "Medium", "High", "Ultra"};
    if (ImGui::Combo("SMAA preset", &preset, presetLabels, IM_ARRAYSIZE(presetLabels))) {
        preset_ = static_cast<smaa_original::Preset>(preset);
    }

    int displayMode = static_cast<int>(displayMode_);
    const char* displayLabels[] = {"Final", "Split", "Difference", "Edges", "Blend weights"};
    if (ImGui::Combo("View", &displayMode, displayLabels, IM_ARRAYSIZE(displayLabels))) {
        displayMode_ = static_cast<DisplayMode>(displayMode);
    }

    if (displayMode_ == DisplayMode::Split) {
        ImGui::SliderFloat("Split", &split_, 0.0f, 1.0f);
    }
    if (displayMode_ == DisplayMode::Difference) {
        ImGui::SliderFloat("Diff scale", &diffScale_, 1.0f, 32.0f);
    }

    const auto timings = selectedTimings();
    ImGui::Text("Mode: %s", aaModeName(aaMode_));
    ImGui::Text("%s GPU ms: %.3f total", aaModeName(aaMode_), timings.totalMs());
    ImGui::Text("Edge %.3f | Blend %.3f | Neighborhood %.3f", timings.edgeMs, timings.blendMs, timings.neighborhoodMs);
}

void Application::drawScenePanel() {
    ImGui::TextUnformatted("Scene Library");
    ImGui::InputText("Manifest", sceneManifestPath_.data(), sceneManifestPath_.size());
    if (ImGui::Button("Reload scenes")) {
        reloadSceneManifest();
    }

    const auto& scenes = sceneLibrary_.scenes();
    if (scenes.empty()) {
        ImGui::TextWrapped("No scenes registered. Edit assets/scenes/scenes.json after downloading test scenes.");
        return;
    }

    for (int i = 0; i < static_cast<int>(scenes.size()); ++i) {
        const bool selected = selectedScene_ == i;
        if (ImGui::Selectable(scenes[static_cast<size_t>(i)].name.c_str(), selected)) {
            selectedScene_ = i;
        }
    }

    if (selectedScene_ >= 0 && selectedScene_ < static_cast<int>(scenes.size())) {
        const auto& scene = scenes[static_cast<size_t>(selectedScene_)];
        ImGui::Separator();
        ImGui::Text("Root: %s", scene.root.string().c_str());
        ImGui::Text("Asset: %s", scene.asset.string().c_str());
        ImGui::Text("Format: %s", scene.format.c_str());
        ImGui::Text("Camera: %s", scene.defaultCamera.c_str());
        ImGui::Text("Exposure: %.3f", scene.exposure);
        if (!scene.notes.empty()) {
            ImGui::TextWrapped("%s", scene.notes.c_str());
        }
        if (ImGui::Button("Load selected OBJ scene")) {
            loadSelectedScene();
        }
        ImGui::InputInt("Scene width", &sceneRenderWidth_);
        ImGui::InputInt("Scene height", &sceneRenderHeight_);
        sceneRenderWidth_ = std::max(64, sceneRenderWidth_);
        sceneRenderHeight_ = std::max(64, sceneRenderHeight_);
        ImGui::SliderFloat("Yaw", &sceneCamera_.yaw, -3.14159f, 3.14159f);
        ImGui::SliderFloat("Pitch", &sceneCamera_.pitch, -1.3f, 1.3f);
        ImGui::SliderFloat("Distance", &sceneCamera_.distance, 0.8f, 6.0f);
        ImGui::SliderFloat("Exposure", &sceneCamera_.exposure, 0.1f, 4.0f);
    }
}

void Application::loadImageFromUi() {
    std::string error;
    if (!image_.loadFromFile(imagePath_.data(), error)) {
        errorText_ = "Failed to load image: " + error;
        return;
    }

    resizeSmaaTargets(image_.texture().width(), image_.texture().height());
    statusText_ = "Loaded image: " + image_.label();
    errorText_.clear();
    inputMode_ = InputMode::Image;
}

void Application::reloadSceneManifest() {
    std::string error;
    if (!sceneLibrary_.loadManifest(sceneManifestPath_.data(), error)) {
        errorText_ = error;
        return;
    }

    selectedScene_ = sceneLibrary_.scenes().empty() ? -1 : 0;
    statusText_ = "Loaded scene manifest: " + sceneLibrary_.manifestPath().string();
    errorText_.clear();
}

void Application::loadSelectedScene() {
    const auto& scenes = sceneLibrary_.scenes();
    if (selectedScene_ < 0 || selectedScene_ >= static_cast<int>(scenes.size())) {
        errorText_ = "No scene selected.";
        return;
    }

    const auto& scene = scenes[static_cast<size_t>(selectedScene_)];
    const auto format = lowerCopy(scene.format);
    if (!format.empty() && format != "obj") {
        errorText_ = "Only OBJ scenes are supported in this implementation step.";
        return;
    }

    std::string error;
    if (!sceneRenderer_.loadObjScene(scene, sceneLibrary_.manifestPath(), error)) {
        errorText_ = error;
        return;
    }

    sceneCamera_.exposure = scene.exposure;
    inputMode_ = InputMode::Scene;
    statusText_ = "Loaded scene: " + scene.name;
    if (!error.empty()) {
        statusText_ += " (" + error + ")";
    }
    errorText_.clear();
}

void Application::resizeSmaaTargets(int sourceWidth, int sourceHeight) {
    originalSmaa_.resize(sourceWidth, sourceHeight);
    adaptiveSmaa_.resize(sourceWidth, sourceHeight);
    adaptiveTscmaaSmaa_.resize(sourceWidth, sourceHeight);
}

void Application::updateSmaa(GLuint sourceTexture, int sourceWidth, int sourceHeight) {
    if (aaMode_ == AaMode::None) {
        return;
    }

    switch (aaMode_) {
    case AaMode::None:
        break;
    case AaMode::OriginalSmaa:
        originalSmaa_.resize(sourceWidth, sourceHeight);
        originalSmaa_.setPreset(preset_);
        originalSmaa_.execute(sourceTexture);
        break;
    case AaMode::AdaptiveSmaa:
        adaptiveSmaa_.resize(sourceWidth, sourceHeight);
        adaptiveSmaa_.setPreset(preset_);
        adaptiveSmaa_.execute(sourceTexture);
        break;
    case AaMode::AdaptiveTscmaaSmaa:
        adaptiveTscmaaSmaa_.resize(sourceWidth, sourceHeight);
        adaptiveTscmaaSmaa_.setPreset(preset_);
        adaptiveTscmaaSmaa_.execute(sourceTexture);
        break;
    }
}

smaa_original::DebugTextures Application::selectedDebugTextures() const {
    switch (aaMode_) {
    case AaMode::None:
        return {};
    case AaMode::OriginalSmaa:
        return originalSmaa_.debugTextures();
    case AaMode::AdaptiveSmaa:
        return adaptiveSmaa_.debugTextures();
    case AaMode::AdaptiveTscmaaSmaa:
        return adaptiveTscmaaSmaa_.debugTextures();
    }
    return {};
}

smaa_original::Timings Application::selectedTimings() const {
    switch (aaMode_) {
    case AaMode::None:
        return {};
    case AaMode::OriginalSmaa:
        return originalSmaa_.timings();
    case AaMode::AdaptiveSmaa:
        return adaptiveSmaa_.timings();
    case AaMode::AdaptiveTscmaaSmaa:
        return adaptiveTscmaaSmaa_.timings();
    }
    return {};
}

} // namespace app
