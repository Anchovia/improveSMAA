#include "gl/Gl.h"

#include <stdexcept>
#include <string>

void (APIENTRY* glActiveTexture)(GLenum) = nullptr;
void (APIENTRY* glAttachShader)(GLuint, GLuint) = nullptr;
void (APIENTRY* glBeginQuery)(GLenum, GLuint) = nullptr;
void (APIENTRY* glBindBuffer)(GLenum, GLuint) = nullptr;
void (APIENTRY* glBindFramebuffer)(GLenum, GLuint) = nullptr;
void (APIENTRY* glBindRenderbuffer)(GLenum, GLuint) = nullptr;
void (APIENTRY* glBindTexture)(GLenum, GLuint) = nullptr;
void (APIENTRY* glBindVertexArray)(GLuint) = nullptr;
void (APIENTRY* glBufferData)(GLenum, GLsizeiptr, const void*, GLenum) = nullptr;
GLenum (APIENTRY* glCheckFramebufferStatus)(GLenum) = nullptr;
void (APIENTRY* glClear)(GLbitfield) = nullptr;
void (APIENTRY* glClearBufferfv)(GLenum, GLint, const GLfloat*) = nullptr;
void (APIENTRY* glClearColor)(GLfloat, GLfloat, GLfloat, GLfloat) = nullptr;
void (APIENTRY* glCompileShader)(GLuint) = nullptr;
GLuint (APIENTRY* glCreateProgram)() = nullptr;
GLuint (APIENTRY* glCreateShader)(GLenum) = nullptr;
void (APIENTRY* glDeleteBuffers)(GLsizei, const GLuint*) = nullptr;
void (APIENTRY* glDeleteFramebuffers)(GLsizei, const GLuint*) = nullptr;
void (APIENTRY* glDeleteProgram)(GLuint) = nullptr;
void (APIENTRY* glDeleteQueries)(GLsizei, const GLuint*) = nullptr;
void (APIENTRY* glDeleteRenderbuffers)(GLsizei, const GLuint*) = nullptr;
void (APIENTRY* glDeleteShader)(GLuint) = nullptr;
void (APIENTRY* glDeleteTextures)(GLsizei, const GLuint*) = nullptr;
void (APIENTRY* glDeleteVertexArrays)(GLsizei, const GLuint*) = nullptr;
void (APIENTRY* glDisable)(GLenum) = nullptr;
void (APIENTRY* glDrawArrays)(GLenum, GLint, GLsizei) = nullptr;
void (APIENTRY* glDrawBuffer)(GLenum) = nullptr;
void (APIENTRY* glEnable)(GLenum) = nullptr;
void (APIENTRY* glEnableVertexAttribArray)(GLuint) = nullptr;
void (APIENTRY* glEndQuery)(GLenum) = nullptr;
void (APIENTRY* glFramebufferRenderbuffer)(GLenum, GLenum, GLenum, GLuint) = nullptr;
void (APIENTRY* glFramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint) = nullptr;
void (APIENTRY* glGenBuffers)(GLsizei, GLuint*) = nullptr;
void (APIENTRY* glGenFramebuffers)(GLsizei, GLuint*) = nullptr;
void (APIENTRY* glGenQueries)(GLsizei, GLuint*) = nullptr;
void (APIENTRY* glGenRenderbuffers)(GLsizei, GLuint*) = nullptr;
void (APIENTRY* glGenTextures)(GLsizei, GLuint*) = nullptr;
void (APIENTRY* glGenVertexArrays)(GLsizei, GLuint*) = nullptr;
void (APIENTRY* glGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*) = nullptr;
void (APIENTRY* glGetProgramiv)(GLuint, GLenum, GLint*) = nullptr;
void (APIENTRY* glGetQueryObjectui64v)(GLuint, GLenum, GLuint64*) = nullptr;
void (APIENTRY* glGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*) = nullptr;
void (APIENTRY* glGetShaderiv)(GLuint, GLenum, GLint*) = nullptr;
GLint (APIENTRY* glGetUniformLocation)(GLuint, const GLchar*) = nullptr;
void (APIENTRY* glLinkProgram)(GLuint) = nullptr;
void (APIENTRY* glPixelStorei)(GLenum, GLint) = nullptr;
void (APIENTRY* glRenderbufferStorage)(GLenum, GLenum, GLsizei, GLsizei) = nullptr;
void (APIENTRY* glShaderSource)(GLuint, GLsizei, const GLchar* const*, const GLint*) = nullptr;
void (APIENTRY* glTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) = nullptr;
void (APIENTRY* glTexParameteri)(GLenum, GLenum, GLint) = nullptr;
void (APIENTRY* glUniform1f)(GLint, GLfloat) = nullptr;
void (APIENTRY* glUniform1i)(GLint, GLint) = nullptr;
void (APIENTRY* glUniform2f)(GLint, GLfloat, GLfloat) = nullptr;
void (APIENTRY* glUniform4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat) = nullptr;
void (APIENTRY* glUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;
void (APIENTRY* glUseProgram)(GLuint) = nullptr;
void (APIENTRY* glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*) = nullptr;
void (APIENTRY* glViewport)(GLint, GLint, GLsizei, GLsizei) = nullptr;

namespace {

template <typename T>
void load(T& function, GLLoadProc loadProc, const char* name) {
    function = reinterpret_cast<T>(loadProc(name));
    if (function == nullptr) {
        throw std::runtime_error(std::string("Failed to load OpenGL function: ") + name);
    }
}

} // namespace

namespace gl {

bool loadOpenGL(GLLoadProc loadProc) {
    if (loadProc == nullptr) {
        return false;
    }

    load(glActiveTexture, loadProc, "glActiveTexture");
    load(glAttachShader, loadProc, "glAttachShader");
    load(glBeginQuery, loadProc, "glBeginQuery");
    load(glBindBuffer, loadProc, "glBindBuffer");
    load(glBindFramebuffer, loadProc, "glBindFramebuffer");
    load(glBindRenderbuffer, loadProc, "glBindRenderbuffer");
    load(glBindTexture, loadProc, "glBindTexture");
    load(glBindVertexArray, loadProc, "glBindVertexArray");
    load(glBufferData, loadProc, "glBufferData");
    load(glCheckFramebufferStatus, loadProc, "glCheckFramebufferStatus");
    load(glClear, loadProc, "glClear");
    load(glClearBufferfv, loadProc, "glClearBufferfv");
    load(glClearColor, loadProc, "glClearColor");
    load(glCompileShader, loadProc, "glCompileShader");
    load(glCreateProgram, loadProc, "glCreateProgram");
    load(glCreateShader, loadProc, "glCreateShader");
    load(glDeleteBuffers, loadProc, "glDeleteBuffers");
    load(glDeleteFramebuffers, loadProc, "glDeleteFramebuffers");
    load(glDeleteProgram, loadProc, "glDeleteProgram");
    load(glDeleteQueries, loadProc, "glDeleteQueries");
    load(glDeleteRenderbuffers, loadProc, "glDeleteRenderbuffers");
    load(glDeleteShader, loadProc, "glDeleteShader");
    load(glDeleteTextures, loadProc, "glDeleteTextures");
    load(glDeleteVertexArrays, loadProc, "glDeleteVertexArrays");
    load(glDisable, loadProc, "glDisable");
    load(glDrawArrays, loadProc, "glDrawArrays");
    load(glDrawBuffer, loadProc, "glDrawBuffer");
    load(glEnable, loadProc, "glEnable");
    load(glEnableVertexAttribArray, loadProc, "glEnableVertexAttribArray");
    load(glEndQuery, loadProc, "glEndQuery");
    load(glFramebufferRenderbuffer, loadProc, "glFramebufferRenderbuffer");
    load(glFramebufferTexture2D, loadProc, "glFramebufferTexture2D");
    load(glGenBuffers, loadProc, "glGenBuffers");
    load(glGenFramebuffers, loadProc, "glGenFramebuffers");
    load(glGenQueries, loadProc, "glGenQueries");
    load(glGenRenderbuffers, loadProc, "glGenRenderbuffers");
    load(glGenTextures, loadProc, "glGenTextures");
    load(glGenVertexArrays, loadProc, "glGenVertexArrays");
    load(glGetProgramInfoLog, loadProc, "glGetProgramInfoLog");
    load(glGetProgramiv, loadProc, "glGetProgramiv");
    load(glGetQueryObjectui64v, loadProc, "glGetQueryObjectui64v");
    load(glGetShaderInfoLog, loadProc, "glGetShaderInfoLog");
    load(glGetShaderiv, loadProc, "glGetShaderiv");
    load(glGetUniformLocation, loadProc, "glGetUniformLocation");
    load(glLinkProgram, loadProc, "glLinkProgram");
    load(glPixelStorei, loadProc, "glPixelStorei");
    load(glRenderbufferStorage, loadProc, "glRenderbufferStorage");
    load(glShaderSource, loadProc, "glShaderSource");
    load(glTexImage2D, loadProc, "glTexImage2D");
    load(glTexParameteri, loadProc, "glTexParameteri");
    load(glUniform1f, loadProc, "glUniform1f");
    load(glUniform1i, loadProc, "glUniform1i");
    load(glUniform2f, loadProc, "glUniform2f");
    load(glUniform4f, loadProc, "glUniform4f");
    load(glUniformMatrix4fv, loadProc, "glUniformMatrix4fv");
    load(glUseProgram, loadProc, "glUseProgram");
    load(glVertexAttribPointer, loadProc, "glVertexAttribPointer");
    load(glViewport, loadProc, "glViewport");

    return true;
}

} // namespace gl
