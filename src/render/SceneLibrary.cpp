#include "render/SceneLibrary.h"

#include <fstream>
#include <regex>
#include <sstream>
#include <utility>

namespace {

std::string readFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string stringValue(const std::string& objectText, const char* key) {
    const std::regex pattern("\"" + std::string(key) + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(objectText, match, pattern)) {
        return match[1].str();
    }
    return {};
}

float floatValue(const std::string& objectText, const char* key, float fallback) {
    const std::regex pattern("\"" + std::string(key) + "\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
    std::smatch match;
    if (std::regex_search(objectText, match, pattern)) {
        return std::stof(match[1].str());
    }
    return fallback;
}

} // namespace

namespace render {

bool SceneLibrary::loadManifest(const std::filesystem::path& path, std::string& error) {
    manifestPath_ = path;
    scenes_.clear();

    if (!std::filesystem::exists(path)) {
        error = "Scene manifest does not exist: " + path.string();
        return false;
    }

    const auto text = readFile(path);
    const std::regex objectPattern("\\{([^{}]*)\\}");
    auto begin = std::sregex_iterator(text.begin(), text.end(), objectPattern);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        const auto objectText = it->str();
        SceneEntry entry;
        entry.name = stringValue(objectText, "name");
        entry.root = stringValue(objectText, "root");
        entry.asset = stringValue(objectText, "asset");
        entry.format = stringValue(objectText, "format");
        entry.defaultCamera = stringValue(objectText, "default_camera");
        entry.exposure = floatValue(objectText, "exposure", 1.0f);
        entry.notes = stringValue(objectText, "notes");

        if (!entry.name.empty()) {
            scenes_.push_back(std::move(entry));
        }
    }

    error.clear();
    return true;
}

} // namespace render
