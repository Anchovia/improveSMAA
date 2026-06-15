#include "render/SceneRenderer.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <stb_image.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

using Mat4 = std::array<float, 16>;

constexpr int kSceneVertexStride = 8;

Vec3 operator-(Vec3 a, Vec3 b) {
    return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 operator+(Vec3 a, Vec3 b) {
    return Vec3{a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 operator*(Vec3 v, float s) {
    return Vec3{v.x * s, v.y * s, v.z * s};
}

float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 cross(Vec3 a, Vec3 b) {
    return Vec3{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

Vec3 normalize(Vec3 v) {
    const float length = std::sqrt(std::max(0.000001f, dot(v, v)));
    return v * (1.0f / length);
}

Mat4 identity() {
    return Mat4{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
}

Mat4 multiply(const Mat4& a, const Mat4& b) {
    Mat4 result{};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            result[static_cast<size_t>(column * 4 + row)] =
                a[0 * 4 + row] * b[column * 4 + 0] +
                a[1 * 4 + row] * b[column * 4 + 1] +
                a[2 * 4 + row] * b[column * 4 + 2] +
                a[3 * 4 + row] * b[column * 4 + 3];
        }
    }
    return result;
}

Mat4 perspective(float fovyRadians, float aspect, float nearPlane, float farPlane) {
    const float f = 1.0f / std::tan(fovyRadians * 0.5f);
    Mat4 result{};
    result[0] = f / aspect;
    result[5] = f;
    result[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
    result[11] = -1.0f;
    result[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    return result;
}

Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up) {
    const Vec3 f = normalize(center - eye);
    const Vec3 s = normalize(cross(f, up));
    const Vec3 u = cross(s, f);

    Mat4 result = identity();
    result[0] = s.x;
    result[4] = s.y;
    result[8] = s.z;
    result[1] = u.x;
    result[5] = u.y;
    result[9] = u.z;
    result[2] = -f.x;
    result[6] = -f.y;
    result[10] = -f.z;
    result[12] = -dot(s, eye);
    result[13] = -dot(u, eye);
    result[14] = dot(f, eye);
    return result;
}

std::filesystem::path resolveRoot(const render::SceneEntry& entry, const std::filesystem::path& manifestPath) {
    if (entry.root.is_absolute()) {
        return entry.root;
    }
    return manifestPath.parent_path() / entry.root;
}

std::filesystem::path resolveAsset(const render::SceneEntry& entry, const std::filesystem::path& root) {
    if (entry.asset.is_absolute()) {
        return entry.asset;
    }
    return root / entry.asset;
}

Vec3 readPosition(const tinyobj::attrib_t& attrib, tinyobj::index_t index) {
    const int base = 3 * index.vertex_index;
    return Vec3{
        attrib.vertices[static_cast<size_t>(base + 0)],
        attrib.vertices[static_cast<size_t>(base + 1)],
        attrib.vertices[static_cast<size_t>(base + 2)],
    };
}

Vec3 readNormal(const tinyobj::attrib_t& attrib, tinyobj::index_t index, Vec3 fallback) {
    if (index.normal_index < 0) {
        return fallback;
    }
    const int base = 3 * index.normal_index;
    return normalize(Vec3{
        attrib.normals[static_cast<size_t>(base + 0)],
        attrib.normals[static_cast<size_t>(base + 1)],
        attrib.normals[static_cast<size_t>(base + 2)],
    });
}

Vec2 readTexcoord(const tinyobj::attrib_t& attrib, tinyobj::index_t index) {
    if (index.texcoord_index < 0) {
        return {};
    }
    const int base = 2 * index.texcoord_index;
    if (base + 1 >= static_cast<int>(attrib.texcoords.size())) {
        return {};
    }
    return Vec2{
        attrib.texcoords[static_cast<size_t>(base + 0)],
        attrib.texcoords[static_cast<size_t>(base + 1)],
    };
}

Vec3 materialColor(const std::vector<tinyobj::material_t>& materials, int materialId) {
    if (materialId >= 0 && materialId < static_cast<int>(materials.size())) {
        const auto& material = materials[static_cast<size_t>(materialId)];
        return Vec3{material.diffuse[0], material.diffuse[1], material.diffuse[2]};
    }
    return Vec3{0.74f, 0.76f, 0.78f};
}

std::string textureFileName(std::string text) {
    std::replace(text.begin(), text.end(), '\\', '/');
    if (text.find_first_of(" \t\r\n") == std::string::npos) {
        return text;
    }

    std::istringstream stream(text);
    std::string token;
    std::string lastToken;
    while (stream >> token) {
        lastToken = token;
    }
    return lastToken;
}

std::filesystem::path resolveTexturePath(const std::filesystem::path& root, const std::string& textureName) {
    const auto cleanName = textureFileName(textureName);
    const std::filesystem::path texturePath(cleanName);
    if (texturePath.is_absolute()) {
        return texturePath;
    }
    return root / texturePath;
}

void appendWarning(std::string& warning, const std::string& text) {
    if (text.empty()) {
        return;
    }
    if (!warning.empty()) {
        warning += "\n";
    }
    warning += text;
}

std::string filterReaderWarnings(const std::string& text) {
    std::istringstream stream(text);
    std::string line;
    std::string filtered;
    bool skipDissolveFollowup = false;
    int suppressedDissolveWarnings = 0;

    while (std::getline(stream, line)) {
        if (line.find("Both `d` and `Tr`") != std::string::npos) {
            skipDissolveFollowup = true;
            ++suppressedDissolveWarnings;
            continue;
        }
        if (skipDissolveFollowup && line.find("Use the value of `d`") != std::string::npos) {
            skipDissolveFollowup = false;
            continue;
        }

        skipDissolveFollowup = false;
        appendWarning(filtered, line);
    }

    if (suppressedDissolveWarnings > 0) {
        appendWarning(filtered, "Suppressed " + std::to_string(suppressedDissolveWarnings) + " duplicate MTL dissolve warnings.");
    }
    return filtered;
}

bool loadRgbaTexture(const std::filesystem::path& path, gl::Texture2D& texture, std::string& warning) {
    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_set_flip_vertically_on_load(1);
    stbi_uc* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
    stbi_set_flip_vertically_on_load(0);

    if (pixels == nullptr) {
        const char* reason = stbi_failure_reason();
        warning = "Failed to load diffuse texture " + path.string();
        if (reason != nullptr) {
            warning += ": ";
            warning += reason;
        }
        return false;
    }

    texture.createRgba8(width, height, pixels);
    texture.setLinearRepeat();
    texture.setLabel(path.filename().string());
    stbi_image_free(pixels);
    warning.clear();
    return true;
}

} // namespace

namespace render {

SceneRenderer::~SceneRenderer() {
    release();
}

void SceneRenderer::initialize(const std::filesystem::path& shaderRoot) {
    program_ = gl::ShaderProgram::fromFiles(
        shaderRoot / "scene" / "scene.vert",
        shaderRoot / "scene" / "scene.frag");

    glGenFramebuffers(1, &fbo_);
    glGenRenderbuffers(1, &depthRbo_);
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    createFramebuffer();
}

void SceneRenderer::release() {
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (depthRbo_ != 0) {
        glDeleteRenderbuffers(1, &depthRbo_);
        depthRbo_ = 0;
    }
    if (fbo_ != 0) {
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }
    program_ = gl::ShaderProgram();
    colorTex_ = gl::Texture2D();
    materials_.clear();
    batches_.clear();
    vertexCount_ = 0;
}

bool SceneRenderer::loadObjScene(
    const SceneEntry& entry,
    const std::filesystem::path& manifestPath,
    std::string& error) {
    const auto root = resolveRoot(entry, manifestPath);
    const auto asset = resolveAsset(entry, root);

    tinyobj::ObjReaderConfig config;
    config.mtl_search_path = root.string();
    config.triangulate = true;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(asset.string(), config)) {
        error = reader.Error().empty() ? "Failed to parse OBJ scene" : reader.Error();
        return false;
    }

    std::string warnings = filterReaderWarnings(reader.Warning());

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();
    const auto& materials = reader.GetMaterials();

    std::vector<SceneMaterial> loadedMaterials;
    loadedMaterials.reserve(materials.size() + 1);
    for (int materialId = 0; materialId < static_cast<int>(materials.size()); ++materialId) {
        const auto& sourceMaterial = materials[static_cast<size_t>(materialId)];
        SceneMaterial material;
        const Vec3 diffuse = materialColor(materials, materialId);
        material.diffuseColor = {diffuse.x, diffuse.y, diffuse.z};

        if (!sourceMaterial.diffuse_texname.empty()) {
            const auto texturePath = resolveTexturePath(root, sourceMaterial.diffuse_texname);
            std::string textureWarning;
            material.hasDiffuseTexture = loadRgbaTexture(texturePath, material.diffuseTexture, textureWarning);
            appendWarning(warnings, textureWarning);
        }

        loadedMaterials.push_back(std::move(material));
    }

    SceneMaterial fallbackMaterial;
    loadedMaterials.push_back(std::move(fallbackMaterial));
    const int fallbackMaterialIndex = static_cast<int>(loadedMaterials.size() - 1);

    std::vector<float> vertices;
    vertices.reserve((attrib.vertices.size() / 3) * kSceneVertexStride);
    std::vector<MeshBatch> loadedBatches;

    bool boundsInitialized = false;
    Bounds bounds;

    for (const auto& shape : shapes) {
        size_t indexOffset = 0;
        for (size_t face = 0; face < shape.mesh.num_face_vertices.size(); ++face) {
            const int fv = shape.mesh.num_face_vertices[face];
            if (fv != 3) {
                indexOffset += static_cast<size_t>(fv);
                continue;
            }

            const auto i0 = shape.mesh.indices[indexOffset + 0];
            const auto i1 = shape.mesh.indices[indexOffset + 1];
            const auto i2 = shape.mesh.indices[indexOffset + 2];
            const Vec3 p0 = readPosition(attrib, i0);
            const Vec3 p1 = readPosition(attrib, i1);
            const Vec3 p2 = readPosition(attrib, i2);
            const Vec3 faceNormal = normalize(cross(p1 - p0, p2 - p0));
            const int faceMaterialId = face < shape.mesh.material_ids.size() ? shape.mesh.material_ids[face] : -1;
            const int materialIndex =
                faceMaterialId >= 0 && faceMaterialId < static_cast<int>(materials.size()) ? faceMaterialId : fallbackMaterialIndex;

            if (loadedBatches.empty() || loadedBatches.back().materialIndex != materialIndex) {
                loadedBatches.push_back(MeshBatch{
                    static_cast<GLsizei>(vertices.size() / kSceneVertexStride),
                    0,
                    materialIndex,
                });
            }

            const std::array<tinyobj::index_t, 3> indices = {i0, i1, i2};
            const std::array<Vec3, 3> positions = {p0, p1, p2};
            for (size_t v = 0; v < 3; ++v) {
                const Vec3 normal = readNormal(attrib, indices[v], faceNormal);
                const Vec3 position = positions[v];
                const Vec2 texcoord = readTexcoord(attrib, indices[v]);

                if (!boundsInitialized) {
                    bounds = Bounds{position.x, position.y, position.z, position.x, position.y, position.z};
                    boundsInitialized = true;
                } else {
                    bounds.minX = std::min(bounds.minX, position.x);
                    bounds.minY = std::min(bounds.minY, position.y);
                    bounds.minZ = std::min(bounds.minZ, position.z);
                    bounds.maxX = std::max(bounds.maxX, position.x);
                    bounds.maxY = std::max(bounds.maxY, position.y);
                    bounds.maxZ = std::max(bounds.maxZ, position.z);
                }

                vertices.insert(vertices.end(), {
                    position.x, position.y, position.z,
                    normal.x, normal.y, normal.z,
                    texcoord.x, texcoord.y,
                });
            }

            loadedBatches.back().vertexCount += 3;
            indexOffset += 3;
        }
    }

    if (vertices.empty()) {
        error = "OBJ scene has no renderable triangles: " + asset.string();
        return false;
    }

    bounds_ = bounds;
    label_ = entry.name;
    materials_ = std::move(loadedMaterials);
    batches_ = std::move(loadedBatches);
    uploadVertices(vertices);
    error = std::move(warnings);
    return true;
}

void SceneRenderer::resize(int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }
    if (width_ == width && height_ == height && colorTex_.valid()) {
        return;
    }

    width_ = width;
    height_ = height;
    createFramebuffer();
}

void SceneRenderer::render(const SceneCamera& camera) {
    if (vertexCount_ == 0) {
        return;
    }

    const Vec3 boundsCenter{
        (bounds_.minX + bounds_.maxX) * 0.5f,
        (bounds_.minY + bounds_.maxY) * 0.5f,
        (bounds_.minZ + bounds_.maxZ) * 0.5f,
    };
    const Vec3 extent{
        bounds_.maxX - bounds_.minX,
        bounds_.maxY - bounds_.minY,
        bounds_.maxZ - bounds_.minZ,
    };
    const float radius = std::max({extent.x, extent.y, extent.z, 0.001f}) * 0.5f;
    const Vec3 target = boundsCenter + Vec3{
        camera.targetOffsetX,
        camera.targetOffsetY,
        camera.targetOffsetZ,
    } * radius;
    const float cp = std::cos(camera.pitch);
    const Vec3 orbitOffset{
        std::sin(camera.yaw) * cp,
        std::sin(camera.pitch),
        std::cos(camera.yaw) * cp,
    };
    const Vec3 eye = target + orbitOffset * (radius * camera.distance);

    const Mat4 view = lookAt(eye, target, Vec3{0.0f, 1.0f, 0.0f});
    const float fovRadians = std::clamp(camera.fovDegrees, 25.0f, 100.0f) * 3.14159265f / 180.0f;
    const Mat4 proj = perspective(fovRadians, static_cast<float>(width_) / static_cast<float>(height_), radius * 0.005f, radius * 20.0f);
    const Mat4 mvp = multiply(proj, view);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, width_, height_);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(0.025f, 0.027f, 0.032f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    program_.use();
    glUniformMatrix4fv(glGetUniformLocation(program_.id(), "u_mvp"), 1, GL_FALSE, mvp.data());
    program_.setFloat("u_exposure", camera.exposure);
    program_.setInt("u_diffuseTex", 0);

    glBindVertexArray(vao_);
    for (const auto& batch : batches_) {
        const auto materialIndex =
            batch.materialIndex >= 0 && batch.materialIndex < static_cast<int>(materials_.size()) ? batch.materialIndex : 0;
        const auto& material = materials_[static_cast<size_t>(materialIndex)];
        program_.setVec4(
            "u_materialColor",
            material.diffuseColor[0],
            material.diffuseColor[1],
            material.diffuseColor[2],
            1.0f);
        program_.setInt("u_useDiffuseTexture", material.hasDiffuseTexture ? 1 : 0);
        if (material.hasDiffuseTexture) {
            material.diffuseTexture.bind(0);
        }
        glDrawArrays(GL_TRIANGLES, batch.firstVertex, batch.vertexCount);
    }
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

SceneCamera SceneRenderer::defaultCamera(float exposure) const {
    SceneCamera camera{};
    camera.exposure = exposure;

    if (vertexCount_ == 0) {
        return camera;
    }

    const float fovRadians = camera.fovDegrees * 3.14159265f / 180.0f;
    const float fitDistance = 1.25f / std::tan(fovRadians * 0.5f);

    camera.yaw = 0.75f;
    camera.pitch = 0.22f;
    camera.distance = std::clamp(fitDistance, 1.4f, 4.0f);
    camera.targetOffsetX = 0.0f;
    camera.targetOffsetY = 0.0f;
    camera.targetOffsetZ = 0.0f;
    return camera;
}

void SceneRenderer::createFramebuffer() {
    colorTex_.createRgba8(width_, height_);
    colorTex_.setLabel("Scene color");

    glBindRenderbuffer(GL_RENDERBUFFER, depthRbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width_, height_);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex_.id(), 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRbo_);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Scene framebuffer is incomplete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneRenderer::uploadVertices(const std::vector<float>& vertices) {
    vertexCount_ = static_cast<GLsizei>(vertices.size() / kSceneVertexStride);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);

    constexpr GLsizei stride = kSceneVertexStride * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(6 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace render
