#pragma once

#include "gl/Texture2D.h"

#include <filesystem>
#include <string>

namespace render {

class ImageSource {
public:
    void createDefaultPattern(int width, int height);
    bool loadFromFile(const std::filesystem::path& path, std::string& error);
    void release();

    const gl::Texture2D& texture() const { return texture_; }
    gl::Texture2D& texture() { return texture_; }
    const std::string& label() const { return label_; }

private:
    gl::Texture2D texture_;
    std::string label_ = "Generated test pattern";
};

} // namespace render
