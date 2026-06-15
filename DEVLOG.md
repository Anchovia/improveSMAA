# Devlog

## 2026-06-15

Started the project as documentation-first.

Created the initial operating documents:

- `README.md`
- `DESIGN.md`
- `DEVLOG.md`

Reference notes:

- `iryoku/smaa` is the source of truth for Original SMAA behavior.
- Original SMAA uses a three-pass post-process path: edge detection, blending weight calculation, and neighborhood blending.
- Original SMAA requires the precomputed area/search textures from upstream.
- Upstream presets include low, medium, high, and ultra.
- `GameTechDev/CMAA2` is useful as a demo reference for static images, AA mode switching, zoom inspection, compare tools, and benchmarking UI.
- `casual-effects.com/data` was added as a scene/test-data reference for user-added benchmark scenes.

Project decisions made so far:

- Use OpenGL.
- Use CMake.
- Build Original SMAA first.
- Keep Original SMAA isolated from Improved SMAA.
- Support a scene-library workflow for downloaded test scenes.
- Support OBJ scene rendering first with `tinyobjloader`.
- Do not run builds from the agent side.

Implementation added:

- CMake project and FetchContent dependency setup.
- GLFW/OpenGL 3.3 application shell with an in-repo OpenGL function loader.
- Dear ImGui control panel.
- Generated static image source and `stb_image` loading.
- Original SMAA wrapper around upstream `SMAA.hlsl`.
- SMAA lookup texture upload from upstream `AreaTex.h` and `SearchTex.h`.
- Final, split, difference, edge, and blend-weight views.
- Scene manifest reader.
- Minimal OBJ renderer that outputs a scene color texture for SMAA testing.
- Fixed the tinyobjloader FetchContent reference from the removed `master` branch to a pinned `release` commit.
- Removed GLAD code generation and added a minimal in-repo OpenGL function loader to avoid Python `jinja2` build dependency.
- Fixed shader include preprocessing so commented `#include` examples in upstream `SMAA.hlsl` are ignored.
- Added Adaptive SMAA and Adaptive + TSCMAA SMAA placeholder modes using copied SMAA 1x shader wrappers.

Remaining decisions:

- OpenGL 3.3 core or 4.5 core long-term.
- Vendored dependencies, CMake `FetchContent`, or system packages long-term.
- Exact upstream SMAA update/import policy.
- Scene registry location and metadata format.
- Whether to add Assimp/glTF after OBJ support is validated.

Next planned step:

User build validation, then shader/runtime fixes if needed.
