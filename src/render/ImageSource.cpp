#include "render/ImageSource.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {

void putPixel(std::vector<std::uint8_t>& pixels, int width, int x, int y, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    const auto index = static_cast<size_t>((y * width + x) * 4);
    pixels[index + 0] = r;
    pixels[index + 1] = g;
    pixels[index + 2] = b;
    pixels[index + 3] = 255;
}

bool nearLine(int x, int y, float slope, float intercept, float thickness) {
    const float yf = static_cast<float>(y);
    const float xf = static_cast<float>(x);
    return std::abs(yf - (slope * xf + intercept)) <= thickness;
}

} // namespace

namespace render {

void ImageSource::createDefaultPattern(int width, int height) {
    std::vector<std::uint8_t> pixels(static_cast<size_t>(width * height * 4));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float u = static_cast<float>(x) / static_cast<float>(std::max(1, width - 1));
            const float v = static_cast<float>(y) / static_cast<float>(std::max(1, height - 1));

            std::uint8_t r = static_cast<std::uint8_t>(40 + 80 * u);
            std::uint8_t g = static_cast<std::uint8_t>(42 + 70 * v);
            std::uint8_t b = static_cast<std::uint8_t>(52 + 80 * (1.0f - u));

            const bool grid = (x % 64 == 0) || (y % 64 == 0);
            if (grid) {
                r = g = b = 95;
            }

            if (nearLine(x, y, -0.42f, static_cast<float>(height) * 0.82f, 1.25f) ||
                nearLine(x, y, 0.58f, static_cast<float>(height) * 0.12f, 1.25f)) {
                r = g = b = 235;
            }

            if (x > width / 8 && x < width / 8 + width / 5 && y > height / 5 && y < height / 5 + height / 4) {
                r = 215;
                g = 65;
                b = 50;
            }

            if (x > width / 2 && x < width / 2 + width / 4 && y > height / 2 && y < height / 2 + height / 5) {
                r = 50;
                g = 160;
                b = 220;
            }

            if ((x / 18 + y / 18) % 2 == 0 && x > width * 3 / 5 && y < height / 3) {
                r = 245;
                g = 245;
                b = 238;
            }

            putPixel(pixels, width, x, y, r, g, b);
        }
    }

    texture_.createRgba8(width, height, pixels.data());
    texture_.setLabel("Generated test pattern");
    label_ = "Generated test pattern";
}

bool ImageSource::loadFromFile(const std::filesystem::path& path, std::string& error) {
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
    if (pixels == nullptr) {
        error = stbi_failure_reason() != nullptr ? stbi_failure_reason() : "unknown image loading error";
        return false;
    }

    texture_.createRgba8(width, height, pixels);
    texture_.setLabel(path.filename().string());
    stbi_image_free(pixels);

    label_ = path.string();
    error.clear();
    return true;
}

void ImageSource::release() {
    texture_ = gl::Texture2D();
}

} // namespace render
