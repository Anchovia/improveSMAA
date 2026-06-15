#pragma once

#include <cstddef>
#include <cstdint>

#ifndef APIENTRY
#define APIENTRY __stdcall
#endif

using GLenum = unsigned int;
using GLboolean = unsigned char;
using GLbitfield = unsigned int;
using GLvoid = void;
using GLbyte = signed char;
using GLshort = short;
using GLint = int;
using GLsizei = int;
using GLubyte = unsigned char;
using GLushort = unsigned short;
using GLuint = unsigned int;
using GLfloat = float;
using GLclampf = float;
using GLdouble = double;
using GLsizeiptr = std::ptrdiff_t;
using GLuint64 = std::uint64_t;
using GLchar = char;

#define GL_FALSE 0
#define GL_TRUE 1

#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406

#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_TRIANGLES 0x0004
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_RGBA 0x1908
#define GL_RED 0x1903
#define GL_DEPTH_TEST 0x0B71
#define GL_BLEND 0x0BE2
#define GL_COLOR 0x1800
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_RGBA8 0x8058
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_FRAMEBUFFER 0x8D40
#define GL_RENDERBUFFER 0x8D41
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_RG 0x8227
#define GL_RG8 0x822B
#define GL_R8 0x8229
#define GL_TIME_ELAPSED 0x88BF
#define GL_QUERY_RESULT 0x8866

using GLProc = void (*)();
using GLLoadProc = GLProc (*)(const char* name);

extern void (APIENTRY* glActiveTexture)(GLenum texture);
extern void (APIENTRY* glAttachShader)(GLuint program, GLuint shader);
extern void (APIENTRY* glBeginQuery)(GLenum target, GLuint id);
extern void (APIENTRY* glBindBuffer)(GLenum target, GLuint buffer);
extern void (APIENTRY* glBindFramebuffer)(GLenum target, GLuint framebuffer);
extern void (APIENTRY* glBindRenderbuffer)(GLenum target, GLuint renderbuffer);
extern void (APIENTRY* glBindTexture)(GLenum target, GLuint texture);
extern void (APIENTRY* glBindVertexArray)(GLuint array);
extern void (APIENTRY* glBufferData)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
extern GLenum (APIENTRY* glCheckFramebufferStatus)(GLenum target);
extern void (APIENTRY* glClear)(GLbitfield mask);
extern void (APIENTRY* glClearBufferfv)(GLenum buffer, GLint drawbuffer, const GLfloat* value);
extern void (APIENTRY* glClearColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void (APIENTRY* glCompileShader)(GLuint shader);
extern GLuint (APIENTRY* glCreateProgram)();
extern GLuint (APIENTRY* glCreateShader)(GLenum type);
extern void (APIENTRY* glDeleteBuffers)(GLsizei n, const GLuint* buffers);
extern void (APIENTRY* glDeleteFramebuffers)(GLsizei n, const GLuint* framebuffers);
extern void (APIENTRY* glDeleteProgram)(GLuint program);
extern void (APIENTRY* glDeleteQueries)(GLsizei n, const GLuint* ids);
extern void (APIENTRY* glDeleteRenderbuffers)(GLsizei n, const GLuint* renderbuffers);
extern void (APIENTRY* glDeleteShader)(GLuint shader);
extern void (APIENTRY* glDeleteTextures)(GLsizei n, const GLuint* textures);
extern void (APIENTRY* glDeleteVertexArrays)(GLsizei n, const GLuint* arrays);
extern void (APIENTRY* glDisable)(GLenum cap);
extern void (APIENTRY* glDrawArrays)(GLenum mode, GLint first, GLsizei count);
extern void (APIENTRY* glDrawBuffer)(GLenum buf);
extern void (APIENTRY* glEnable)(GLenum cap);
extern void (APIENTRY* glEnableVertexAttribArray)(GLuint index);
extern void (APIENTRY* glEndQuery)(GLenum target);
extern void (APIENTRY* glFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
extern void (APIENTRY* glFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern void (APIENTRY* glGenBuffers)(GLsizei n, GLuint* buffers);
extern void (APIENTRY* glGenFramebuffers)(GLsizei n, GLuint* framebuffers);
extern void (APIENTRY* glGenQueries)(GLsizei n, GLuint* ids);
extern void (APIENTRY* glGenRenderbuffers)(GLsizei n, GLuint* renderbuffers);
extern void (APIENTRY* glGenTextures)(GLsizei n, GLuint* textures);
extern void (APIENTRY* glGenVertexArrays)(GLsizei n, GLuint* arrays);
extern void (APIENTRY* glGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
extern void (APIENTRY* glGetProgramiv)(GLuint program, GLenum pname, GLint* params);
extern void (APIENTRY* glGetQueryObjectui64v)(GLuint id, GLenum pname, GLuint64* params);
extern void (APIENTRY* glGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
extern void (APIENTRY* glGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
extern GLint (APIENTRY* glGetUniformLocation)(GLuint program, const GLchar* name);
extern void (APIENTRY* glLinkProgram)(GLuint program);
extern void (APIENTRY* glPixelStorei)(GLenum pname, GLint param);
extern void (APIENTRY* glRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRY* glShaderSource)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
extern void (APIENTRY* glTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);
extern void (APIENTRY* glTexParameteri)(GLenum target, GLenum pname, GLint param);
extern void (APIENTRY* glUniform1f)(GLint location, GLfloat v0);
extern void (APIENTRY* glUniform1i)(GLint location, GLint v0);
extern void (APIENTRY* glUniform2f)(GLint location, GLfloat v0, GLfloat v1);
extern void (APIENTRY* glUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
extern void (APIENTRY* glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
extern void (APIENTRY* glUseProgram)(GLuint program);
extern void (APIENTRY* glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
extern void (APIENTRY* glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);

namespace gl {

bool loadOpenGL(GLLoadProc loadProc);

} // namespace gl
