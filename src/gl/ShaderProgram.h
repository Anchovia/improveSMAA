#pragma once

#include "gl/Gl.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace gl {

class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    static ShaderProgram fromFiles(
        const std::filesystem::path& vertexPath,
        const std::filesystem::path& fragmentPath,
        const std::vector<std::filesystem::path>& includeDirs = {},
        std::string_view vertexPreamble = {},
        std::string_view fragmentPreamble = {});

    void use() const;
    GLuint id() const { return program_; }

    void setInt(const char* name, GLint value) const;
    void setFloat(const char* name, GLfloat value) const;
    void setVec2(const char* name, GLfloat x, GLfloat y) const;
    void setVec4(const char* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const;

private:
    explicit ShaderProgram(GLuint program) : program_(program) {}

    GLuint program_ = 0;
};

} // namespace gl
