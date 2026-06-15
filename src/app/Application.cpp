#include "app/Application.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <system_error>

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

bool isSupportedImageFile(const std::filesystem::path& path) {
    const auto extension = lowerCopy(path.extension().string());
    return extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp" || extension == ".tga";
}

float clampFloat(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

struct OrbitBasis {
    float forwardX = 0.0f;
    float forwardY = 0.0f;
    float forwardZ = 0.0f;
    float rightX = 0.0f;
    float rightZ = 0.0f;
};

OrbitBasis orbitBasis(float yaw, float pitch) {
    const float cp = std::cos(pitch);
    const float orbitX = std::sin(yaw) * cp;
    const float orbitY = std::sin(pitch);
    const float orbitZ = std::cos(yaw) * cp;
    return OrbitBasis{
        -orbitX,
        -orbitY,
        -orbitZ,
        std::cos(yaw),
        -std::sin(yaw),
    };
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

    const auto defaultSceneManifest = rootDir_ / "assets" / "scenes" / "scenes.json";
    setBuffer(sceneManifestPath_, defaultSceneManifest.string());

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
    }

    fullscreenView_.initialize(shaderRoot_);
    sceneRenderer_.initialize(shaderRoot_);
    originalSmaa_.initialize(shaderRoot_, smaaRoot_);
    adaptiveSmaa_.initialize(shaderRoot_, smaaRoot_);
    adaptiveTscmaaSmaa_.initialize(shaderRoot_, smaaRoot_);
    resizeSmaaTargets(image_.texture().width(), image_.texture().height());
    reloadSceneManifest();
    if (argc > 1) {
        loadImageFromUi();
        loadedSource_ = -1;
    }
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
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    processPendingSourceLoad();
    updateSceneCameraInput();

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

    drawUi();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Application::drawUi() {
    ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420.0f, 500.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("improveSMAA");

    drawSourcePanel();
    ImGui::Separator();
    drawAaPanel();
    ImGui::Separator();
    drawInspectPanel();
    ImGui::Separator();
    drawStatsPanel();
    ImGui::Separator();
    drawAdvancedPanel();

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

void Application::drawSourcePanel() {
    ImGui::TextUnformatted("Source");

    if (sourceEntries_.empty()) {
        rebuildSourceEntries();
    }

    if (!sourceEntries_.empty()) {
        selectedSource_ = std::clamp(selectedSource_, 0, static_cast<int>(sourceEntries_.size()) - 1);
        const char* preview = sourceEntries_[static_cast<size_t>(selectedSource_)].label.c_str();
        if (ImGui::BeginCombo("Source", preview)) {
            for (int i = 0; i < static_cast<int>(sourceEntries_.size()); ++i) {
                const bool selected = selectedSource_ == i;
                if (ImGui::Selectable(sourceEntries_[static_cast<size_t>(i)].label.c_str(), selected)) {
                    selectedSource_ = i;
                    if (sourceEntries_[static_cast<size_t>(i)].type == SourceType::Scene) {
                        selectedScene_ = sourceEntries_[static_cast<size_t>(i)].sceneIndex;
                    }
                    requestLoadSelectedSource();
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    } else {
        ImGui::TextUnformatted("No sources");
    }

    if (ImGui::Button("Reload sources")) {
        reloadSceneManifest();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload source")) {
        requestLoadSelectedSource();
    }

    if (pendingSourceLoad_) {
        ImGui::SameLine();
        ImGui::TextDisabled("Loading...");
    }

    if (loadedSource_ >= 0 && loadedSource_ < static_cast<int>(sourceEntries_.size())) {
        ImGui::Text("Loaded: %s", sourceEntries_[static_cast<size_t>(loadedSource_)].label.c_str());
    } else if (inputMode_ == InputMode::Scene) {
        ImGui::Text("Loaded: %s", sceneRenderer_.hasScene() ? sceneRenderer_.label().c_str() : "No scene");
    } else {
        ImGui::Text("Loaded: %s", image_.label().c_str());
    }
}

void Application::drawAaPanel() {
    ImGui::TextUnformatted("AA");

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
}

void Application::drawInspectPanel() {
    ImGui::TextUnformatted("Inspect");

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
}

void Application::drawStatsPanel() {
    ImGui::TextUnformatted("Stats");

    const auto timings = selectedTimings();
    ImGui::Text("Mode: %s", aaModeName(aaMode_));
    ImGui::Text("%s GPU ms: %.3f total", aaModeName(aaMode_), timings.totalMs());
    ImGui::Text("Edge %.3f | Blend %.3f | Neighborhood %.3f", timings.edgeMs, timings.blendMs, timings.neighborhoodMs);

    if (inputMode_ == InputMode::Scene && sceneRenderer_.hasScene()) {
        ImGui::Text("Source: %dx%d scene", sceneRenderer_.width(), sceneRenderer_.height());
    } else {
        ImGui::Text("Source: %dx%d image", image_.texture().width(), image_.texture().height());
    }
}

void Application::drawAdvancedPanel() {
    if (!ImGui::CollapsingHeader("Advanced")) {
        return;
    }

    const bool selectedSourceIsScene =
        selectedSource_ >= 0 && selectedSource_ < static_cast<int>(sourceEntries_.size()) &&
        sourceEntries_[static_cast<size_t>(selectedSource_)].type == SourceType::Scene;
    const bool selectedSourceIsImage = !selectedSourceIsScene;
    if ((inputMode_ == InputMode::Image || selectedSourceIsImage) && ImGui::TreeNode("Image")) {
        ImGui::InputText("Image path", imagePath_.data(), imagePath_.size());
        if (ImGui::Button("Load image")) {
            if (loadImageFromUi()) {
                loadedSource_ = -1;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Generated pattern")) {
            selectedSource_ = 0;
            requestLoadSelectedSource();
        }
        ImGui::TreePop();
    }

    const bool showSceneAdvanced = inputMode_ == InputMode::Scene || selectedSourceIsScene;
    if (!showSceneAdvanced) {
        return;
    }

    const auto& scenes = sceneLibrary_.scenes();
    if (selectedScene_ >= 0 && selectedScene_ < static_cast<int>(scenes.size())) {
        const auto& scene = scenes[static_cast<size_t>(selectedScene_)];
        if (ImGui::TreeNode("Scene")) {
            if (ImGui::Button("Reload scene")) {
                loadSelectedScene();
            }

            ImGui::InputInt("Scene width", &sceneRenderWidth_);
            ImGui::InputInt("Scene height", &sceneRenderHeight_);
            sceneRenderWidth_ = std::max(64, sceneRenderWidth_);
            sceneRenderHeight_ = std::max(64, sceneRenderHeight_);

            if (sceneRenderer_.hasScene()) {
                if (ImGui::Button("Reset camera")) {
                    resetSceneCamera(scene.exposure);
                }
                ImGui::SliderFloat("Yaw", &sceneCamera_.yaw, -3.14159f, 3.14159f);
                ImGui::SliderFloat("Pitch", &sceneCamera_.pitch, -1.3f, 1.3f);
                ImGui::SliderFloat("Distance", &sceneCamera_.distance, 0.03f, 64.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
                ImGui::SliderFloat("Target X", &sceneCamera_.targetOffsetX, -3.0f, 3.0f);
                ImGui::SliderFloat("Target Y", &sceneCamera_.targetOffsetY, -1.5f, 2.0f);
                ImGui::SliderFloat("Target Z", &sceneCamera_.targetOffsetZ, -3.0f, 3.0f);
                ImGui::SliderFloat("Pan speed", &sceneCamera_.panSpeed, 0.05f, 2.0f);
                ImGui::SliderFloat("Zoom speed", &sceneCamera_.zoomSpeed, 0.02f, 0.30f);
                ImGui::SliderFloat("Orbit sensitivity", &sceneCamera_.orbitSensitivity, 0.0005f, 0.006f, "%.4f");
                ImGui::SliderFloat("FOV", &sceneCamera_.fovDegrees, 25.0f, 100.0f);
                ImGui::SliderFloat("Exposure", &sceneCamera_.exposure, 0.1f, 4.0f);
                ImGui::Checkbox("Mouse orbit", &sceneMouseOrbitEnabled_);
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Scene info")) {
            ImGui::Text("Root: %s", scene.root.string().c_str());
            ImGui::Text("Asset: %s", scene.asset.string().c_str());
            ImGui::Text("Format: %s", scene.format.c_str());
            ImGui::Text("Camera: %s", scene.defaultCamera.c_str());
            ImGui::Text("Exposure: %.3f", scene.exposure);
            if (!scene.notes.empty()) {
                ImGui::TextWrapped("%s", scene.notes.c_str());
            }
            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("Scene library")) {
        ImGui::InputText("Manifest", sceneManifestPath_.data(), sceneManifestPath_.size());
        if (ImGui::Button("Reload scenes")) {
            reloadSceneManifest();
        }
        if (scenes.empty()) {
            ImGui::TextUnformatted("No scenes registered.");
        } else {
            ImGui::Text("%d scenes registered", static_cast<int>(scenes.size()));
        }
        ImGui::TreePop();
    }
}

void Application::rebuildSourceEntries() {
    std::string previousLabel;
    if (selectedSource_ >= 0 && selectedSource_ < static_cast<int>(sourceEntries_.size())) {
        previousLabel = sourceEntries_[static_cast<size_t>(selectedSource_)].label;
    }
    std::string previousLoadedLabel;
    if (loadedSource_ >= 0 && loadedSource_ < static_cast<int>(sourceEntries_.size())) {
        previousLoadedLabel = sourceEntries_[static_cast<size_t>(loadedSource_)].label;
    }

    sourceEntries_.clear();
    sourceEntries_.push_back({SourceType::GeneratedPattern, "Generated pattern", {}, -1});

    const auto imagesDir = rootDir_ / "assets" / "test_images";
    std::error_code ec;
    if (std::filesystem::exists(imagesDir, ec)) {
        std::vector<std::filesystem::path> imagePaths;
        for (const auto& entry : std::filesystem::directory_iterator(imagesDir, ec)) {
            if (ec) {
                break;
            }
            if (entry.is_regular_file(ec) && isSupportedImageFile(entry.path())) {
                imagePaths.push_back(entry.path());
            }
        }
        std::sort(imagePaths.begin(), imagePaths.end(), [](const auto& a, const auto& b) {
            return lowerCopy(a.filename().string()) < lowerCopy(b.filename().string());
        });
        for (const auto& path : imagePaths) {
            sourceEntries_.push_back({SourceType::ImageFile, "Image / " + path.filename().string(), path, -1});
        }
    }

    const auto& scenes = sceneLibrary_.scenes();
    for (int i = 0; i < static_cast<int>(scenes.size()); ++i) {
        sourceEntries_.push_back({SourceType::Scene, "Scene / " + scenes[static_cast<size_t>(i)].name, {}, i});
    }

    selectedSource_ = 0;
    if (!previousLabel.empty()) {
        for (int i = 0; i < static_cast<int>(sourceEntries_.size()); ++i) {
            if (sourceEntries_[static_cast<size_t>(i)].label == previousLabel) {
                selectedSource_ = i;
                break;
            }
        }
    }

    if (loadedSource_ >= static_cast<int>(sourceEntries_.size())) {
        loadedSource_ = -1;
    }
    if (!previousLoadedLabel.empty()) {
        loadedSource_ = -1;
        for (int i = 0; i < static_cast<int>(sourceEntries_.size()); ++i) {
            if (sourceEntries_[static_cast<size_t>(i)].label == previousLoadedLabel) {
                loadedSource_ = i;
                break;
            }
        }
    }
}

void Application::requestLoadSelectedSource() {
    pendingSourceLoad_ = true;
}

void Application::processPendingSourceLoad() {
    if (!pendingSourceLoad_) {
        return;
    }

    pendingSourceLoad_ = false;
    loadSelectedSource();
}

void Application::loadSelectedSource() {
    if (sourceEntries_.empty()) {
        rebuildSourceEntries();
    }

    if (selectedSource_ < 0 || selectedSource_ >= static_cast<int>(sourceEntries_.size())) {
        errorText_ = "No source selected.";
        loadedSource_ = -1;
        return;
    }

    const auto& source = sourceEntries_[static_cast<size_t>(selectedSource_)];
    switch (source.type) {
    case SourceType::GeneratedPattern:
        image_.createDefaultPattern(1280, 720);
        resizeSmaaTargets(image_.texture().width(), image_.texture().height());
        inputMode_ = InputMode::Image;
        rotatingSceneCamera_ = false;
        statusText_ = "Using generated test pattern.";
        errorText_.clear();
        loadedSource_ = selectedSource_;
        break;
    case SourceType::ImageFile:
        if (loadImagePath(source.path)) {
            loadedSource_ = selectedSource_;
        }
        break;
    case SourceType::Scene:
        selectedScene_ = source.sceneIndex;
        if (loadSelectedScene()) {
            loadedSource_ = selectedSource_;
        }
        break;
    }
}

bool Application::loadImageFromUi() {
    return loadImagePath(imagePath_.data());
}

bool Application::loadImagePath(const std::filesystem::path& path) {
    std::string error;
    if (!image_.loadFromFile(path, error)) {
        errorText_ = "Failed to load image: " + error;
        return false;
    }

    setBuffer(imagePath_, path.string());
    resizeSmaaTargets(image_.texture().width(), image_.texture().height());
    statusText_ = "Loaded image: " + image_.label();
    errorText_.clear();
    inputMode_ = InputMode::Image;
    rotatingSceneCamera_ = false;
    return true;
}

void Application::reloadSceneManifest() {
    std::string previousSceneName;
    const auto& oldScenes = sceneLibrary_.scenes();
    if (selectedScene_ >= 0 && selectedScene_ < static_cast<int>(oldScenes.size())) {
        previousSceneName = oldScenes[static_cast<size_t>(selectedScene_)].name;
    }

    std::string error;
    if (!sceneLibrary_.loadManifest(sceneManifestPath_.data(), error)) {
        errorText_ = error;
        return;
    }

    selectedScene_ = sceneLibrary_.scenes().empty() ? -1 : 0;
    if (!previousSceneName.empty()) {
        const auto& scenes = sceneLibrary_.scenes();
        for (int i = 0; i < static_cast<int>(scenes.size()); ++i) {
            if (scenes[static_cast<size_t>(i)].name == previousSceneName) {
                selectedScene_ = i;
                break;
            }
        }
    }

    rebuildSourceEntries();
    statusText_ = "Loaded scene manifest: " + sceneLibrary_.manifestPath().string();
    errorText_.clear();
}

bool Application::loadSelectedScene() {
    const auto& scenes = sceneLibrary_.scenes();
    if (selectedScene_ < 0 || selectedScene_ >= static_cast<int>(scenes.size())) {
        errorText_ = "No scene selected.";
        return false;
    }

    const auto& scene = scenes[static_cast<size_t>(selectedScene_)];
    const auto format = lowerCopy(scene.format);
    if (!format.empty() && format != "obj") {
        errorText_ = "Only OBJ scenes are supported in this implementation step.";
        return false;
    }

    std::string error;
    if (!sceneRenderer_.loadObjScene(scene, sceneLibrary_.manifestPath(), error)) {
        errorText_ = error;
        return false;
    }

    resetSceneCamera(scene.exposure);
    inputMode_ = InputMode::Scene;
    statusText_ = "Loaded scene: " + scene.name;
    if (!error.empty()) {
        statusText_ += " (" + error + ")";
    }
    errorText_.clear();
    return true;
}

void Application::resetSceneCamera(float exposure) {
    sceneCamera_ = sceneRenderer_.hasScene() ? sceneRenderer_.defaultCamera(exposure) : render::SceneCamera{};
    if (!sceneRenderer_.hasScene()) {
        sceneCamera_.exposure = exposure;
    }
    sceneMouseOrbitEnabled_ = false;
    sceneMiddleMouseWasDown_ = false;
    rotatingSceneCamera_ = false;
}

void Application::updateSceneCameraInput() {
    if (window_ == nullptr) {
        return;
    }

    const double now = glfwGetTime();
    if (lastSceneCameraTime_ <= 0.0) {
        lastSceneCameraTime_ = now;
    }
    const float dt = static_cast<float>(std::min(now - lastSceneCameraTime_, 0.1));
    lastSceneCameraTime_ = now;

    if (inputMode_ != InputMode::Scene || !sceneRenderer_.hasScene()) {
        sceneMouseOrbitEnabled_ = false;
        sceneMiddleMouseWasDown_ = false;
        rotatingSceneCamera_ = false;
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    double cursorX = 0.0;
    double cursorY = 0.0;
    glfwGetCursorPos(window_, &cursorX, &cursorY);

    const bool middleMouseDown = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    if (middleMouseDown && !sceneMiddleMouseWasDown_ && !io.WantCaptureMouse) {
        sceneMouseOrbitEnabled_ = !sceneMouseOrbitEnabled_;
        rotatingSceneCamera_ = false;
        lastSceneCursorX_ = cursorX;
        lastSceneCursorY_ = cursorY;
    }
    sceneMiddleMouseWasDown_ = middleMouseDown;

    if (sceneMouseOrbitEnabled_ && !io.WantCaptureMouse) {
        if (rotatingSceneCamera_) {
            const float dx = static_cast<float>(cursorX - lastSceneCursorX_);
            const float dy = static_cast<float>(cursorY - lastSceneCursorY_);
            sceneCamera_.yaw -= dx * sceneCamera_.orbitSensitivity;
            sceneCamera_.pitch = clampFloat(sceneCamera_.pitch + dy * sceneCamera_.orbitSensitivity, -1.35f, 1.35f);
        }
        rotatingSceneCamera_ = true;
    } else {
        rotatingSceneCamera_ = false;
    }

    lastSceneCursorX_ = cursorX;
    lastSceneCursorY_ = cursorY;

    const OrbitBasis basis = orbitBasis(sceneCamera_.yaw, sceneCamera_.pitch);

    if (io.MouseWheel != 0.0f && !io.WantCaptureMouse) {
        const float zoomBase = clampFloat(1.0f - sceneCamera_.zoomSpeed, 0.5f, 0.98f);
        sceneCamera_.distance *= std::pow(zoomBase, io.MouseWheel);
    }

    const auto clampSceneCamera = [this]() {
        sceneCamera_.distance = clampFloat(sceneCamera_.distance, 0.03f, 64.0f);
        sceneCamera_.targetOffsetX = clampFloat(sceneCamera_.targetOffsetX, -3.0f, 3.0f);
        sceneCamera_.targetOffsetY = clampFloat(sceneCamera_.targetOffsetY, -1.5f, 2.0f);
        sceneCamera_.targetOffsetZ = clampFloat(sceneCamera_.targetOffsetZ, -3.0f, 3.0f);
        sceneCamera_.panSpeed = clampFloat(sceneCamera_.panSpeed, 0.05f, 2.0f);
        sceneCamera_.zoomSpeed = clampFloat(sceneCamera_.zoomSpeed, 0.02f, 0.30f);
        sceneCamera_.orbitSensitivity = clampFloat(sceneCamera_.orbitSensitivity, 0.0005f, 0.006f);
    };

    if (io.WantCaptureKeyboard) {
        clampSceneCamera();
        return;
    }

    const float speed = (glfwGetKey(window_, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                         glfwGetKey(window_, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
                            ? sceneCamera_.panSpeed * 3.0f
                            : sceneCamera_.panSpeed;
    const float step = speed * std::max(dt, 0.001f) * std::max(sceneCamera_.distance, 1.0f);
    const float rotationStep = 1.2f * std::max(dt, 0.001f);

    if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) {
        sceneCamera_.targetOffsetX += basis.forwardX * step;
        sceneCamera_.targetOffsetY += basis.forwardY * step;
        sceneCamera_.targetOffsetZ += basis.forwardZ * step;
    }
    if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS) {
        sceneCamera_.targetOffsetX -= basis.forwardX * step;
        sceneCamera_.targetOffsetY -= basis.forwardY * step;
        sceneCamera_.targetOffsetZ -= basis.forwardZ * step;
    }
    if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) {
        sceneCamera_.targetOffsetX += basis.rightX * step;
        sceneCamera_.targetOffsetZ += basis.rightZ * step;
    }
    if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS) {
        sceneCamera_.targetOffsetX -= basis.rightX * step;
        sceneCamera_.targetOffsetZ -= basis.rightZ * step;
    }
    if (glfwGetKey(window_, GLFW_KEY_Q) == GLFW_PRESS) {
        sceneCamera_.yaw += rotationStep;
    }
    if (glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS) {
        sceneCamera_.yaw -= rotationStep;
    }
    if (glfwGetKey(window_, GLFW_KEY_R) == GLFW_PRESS) {
        resetSceneCamera(sceneCamera_.exposure);
    }

    clampSceneCamera();
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
