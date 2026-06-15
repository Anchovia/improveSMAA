#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace render {

struct SceneEntry {
    std::string name;
    std::filesystem::path root;
    std::filesystem::path asset;
    std::string format;
    std::string defaultCamera;
    float exposure = 1.0f;
    std::string notes;
};

class SceneLibrary {
public:
    bool loadManifest(const std::filesystem::path& path, std::string& error);

    const std::filesystem::path& manifestPath() const { return manifestPath_; }
    const std::vector<SceneEntry>& scenes() const { return scenes_; }

private:
    std::filesystem::path manifestPath_;
    std::vector<SceneEntry> scenes_;
};

} // namespace render
