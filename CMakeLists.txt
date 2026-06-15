cmake_minimum_required(VERSION 3.24)

project(improveSMAA LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(IMPROVESMAA_FETCH_DEPS "Fetch third-party dependencies with CMake" ON)

include(cmake/Dependencies.cmake)

add_executable(improve_smaa
    src/main.cpp
    src/app/Application.cpp
    src/gl/Gl.cpp
    src/gl/ShaderProgram.cpp
    src/gl/Texture2D.cpp
    src/render/FullscreenView.cpp
    src/render/ImageSource.cpp
    src/render/SceneLibrary.cpp
    src/render/SceneRenderer.cpp
    src/smaa_original/OriginalSmaa.cpp
    src/smaa_variant/SmaaVariant.cpp
)

target_include_directories(improve_smaa PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external/iryoku_smaa
)

target_compile_definitions(improve_smaa PRIVATE
    GLFW_INCLUDE_NONE
    IMPROVESMAA_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(improve_smaa PRIVATE
    OpenGL::GL
    glfw
    imgui
    stb
    tinyobjloader
)

if(MSVC)
    target_compile_options(improve_smaa PRIVATE /W4 /permissive-)
else()
    target_compile_options(improve_smaa PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT improve_smaa)
