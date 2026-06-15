#include "gl/ShaderProgram.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace {

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open shader file: " + path.string());
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::filesystem::path resolveInclude(
    const std::string& includeName,
    const std::filesystem::path& parentDir,
    const std::vector<std::filesystem::path>& includeDirs) {
    const auto localPath = parentDir / includeName;
    if (std::filesystem::exists(localPath)) {
        return localPath;
    }

    for (const auto& dir : includeDirs) {
        const auto candidate = dir / includeName;
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    throw std::runtime_error("Failed to resolve shader include: " + includeName);
}

std::string preprocessShader(
    const std::filesystem::path& path,
    const std::vector<std::filesystem::path>& includeDirs,
    std::string_view preamble);

std::string preprocessShaderBody(
    const std::string& source,
    const std::filesystem::path& parentDir,
    const std::vector<std::filesystem::path>& includeDirs) {
    std::istringstream input(source);
    std::ostringstream output;
    std::string line;

    while (std::getline(input, line)) {
        const auto firstNonSpace = line.find_first_not_of(" \t");
        const std::string_view directive =
            firstNonSpace == std::string::npos ? std::string_view{} : std::string_view(line).substr(firstNonSpace);

        if (directive.size() >= 8 && directive.compare(0, 8, "#include") == 0) {
            const auto includePos = firstNonSpace;
            const auto firstQuote = line.find('"', includePos);
            const auto lastQuote = line.find('"', firstQuote + 1);
            if (firstQuote != std::string::npos && lastQuote != std::string::npos) {
                const auto includeName = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                const auto includePath = resolveInclude(includeName, parentDir, includeDirs);
                output << preprocessShader(includePath, includeDirs, {}) << '\n';
                continue;
            }
        }

        output << line << '\n';
    }

    return output.str();
}

std::string insertPreambleAfterVersion(std::string source, std::string_view preamble) {
    if (preamble.empty()) {
        return source;
    }

    if (source.rfind("#version", 0) != 0) {
        return std::string(preamble) + '\n' + source;
    }

    const auto lineEnd = source.find('\n');
    if (lineEnd == std::string::npos) {
        return source + '\n' + std::string(preamble) + '\n';
    }

    source.insert(lineEnd + 1, std::string(preamble) + '\n');
    return source;
}

std::string preprocessShader(
    const std::filesystem::path& path,
    const std::vector<std::filesystem::path>& includeDirs,
    std::string_view preamble) {
    auto source = insertPreambleAfterVersion(readTextFile(path), preamble);
    return preprocessShaderBody(source, path.parent_path(), includeDirs);
}

GLuint compileShader(GLenum type, const std::string& source, const std::filesystem::path& path) {
    const GLuint shader = glCreateShader(type);
    const char* sourcePtr = source.c_str();
    glShaderSource(shader, 1, &sourcePtr, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        return shader;
    }

    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(static_cast<size_t>(logLength), '\0');
    if (logLength > 0) {
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
    }
    glDeleteShader(shader);

    throw std::runtime_error("Shader compile failed for " + path.string() + ":\n" + log);
}

GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader) {
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) {
        return program;
    }

    GLint logLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(static_cast<size_t>(logLength), '\0');
    if (logLength > 0) {
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
    }
    glDeleteProgram(program);

    throw std::runtime_error("Shader link failed:\n" + log);
}

} // namespace

namespace gl {

ShaderProgram::~ShaderProgram() {
    if (program_ != 0) {
        glDeleteProgram(program_);
    }
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept : program_(std::exchange(other.program_, 0)) {}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if (this != &other) {
        if (program_ != 0) {
            glDeleteProgram(program_);
        }
        program_ = std::exchange(other.program_, 0);
    }
    return *this;
}

ShaderProgram ShaderProgram::fromFiles(
    const std::filesystem::path& vertexPath,
    const std::filesystem::path& fragmentPath,
    const std::vector<std::filesystem::path>& includeDirs,
    std::string_view vertexPreamble,
    std::string_view fragmentPreamble) {
    const auto vertexSource = preprocessShader(vertexPath, includeDirs, vertexPreamble);
    const auto fragmentSource = preprocessShader(fragmentPath, includeDirs, fragmentPreamble);

    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource, vertexPath);
    const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource, fragmentPath);
    const GLuint program = linkProgram(vertexShader, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return ShaderProgram(program);
}

void ShaderProgram::use() const {
    glUseProgram(program_);
}

void ShaderProgram::setInt(const char* name, GLint value) const {
    glUniform1i(glGetUniformLocation(program_, name), value);
}

void ShaderProgram::setFloat(const char* name, GLfloat value) const {
    glUniform1f(glGetUniformLocation(program_, name), value);
}

void ShaderProgram::setVec2(const char* name, GLfloat x, GLfloat y) const {
    glUniform2f(glGetUniformLocation(program_, name), x, y);
}

void ShaderProgram::setVec4(const char* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const {
    glUniform4f(glGetUniformLocation(program_, name), x, y, z, w);
}

} // namespace gl
