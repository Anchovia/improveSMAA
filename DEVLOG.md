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
- Added ignored local scene manifest workflow for downloaded Sponza/Gallery test scenes.
- Extended the OBJ renderer to read `vt` coordinates and MTL `map_Kd` diffuse textures, with material-color fallback.
- Filtered noisy OBJ/MTL duplicate dissolve warnings from the scene-load status text.
- Replaced the temporary FPS-style free camera with a third-person orbit camera: target/pivot offsets, yaw/pitch, target distance, middle-mouse orbit toggle, mouse-wheel distance zoom, WASD view-relative target movement, Q/E yaw rotation, Shift move speed boost, and scene-aware reset.
- Replaced the earlier separate scene list with the unified Source selector described below.
- Hid source-specific controls outside their input mode: scene controls are not shown in image mode, and image file controls are not shown in scene mode.
- Simplified the scene panel by keeping manifest and scene metadata in collapsible sections.
- Reworked the main UI around a CMAA2-style Source selector that lists generated pattern, discovered test images, and registered OBJ scenes together.
- Moved image path editing, scene metadata, render size, and camera controls into Advanced so normal image testing is not polluted by scene controls.
- Changed source selection to queue load requests and process them at the start of the next frame instead of loading scenes directly from ImGui selection callbacks.

Remaining decisions:

- OpenGL 3.3 core or 4.5 core long-term.
- Vendored dependencies, CMake `FetchContent`, or system packages long-term.
- Exact upstream SMAA update/import policy.
- Scene registry location and metadata format.
- Whether to add Assimp/glTF after OBJ support is validated.

Next planned step:

User build validation of the Source selector UI, then shader/runtime fixes if needed.
