# improveSMAA

OpenGL/CMake demo project for testing improvements to SMAA while keeping the original SMAA implementation as a stable measurement baseline.

This repository is starting documentation-first. The first implementation milestone is to run Original SMAA correctly. Improved SMAA should only be added after the baseline path is reproducible and measurable.

## Goals

- Build an OpenGL demo that can compare anti-aliasing modes from the same input frame.
- Implement Original SMAA first, based as directly as possible on `iryoku/smaa`.
- Keep Original SMAA immutable except for narrow OpenGL integration wrappers.
- Add Improved SMAA as a separate implementation with the same input/output contract.
- Use an ImGui-style UI for mode selection, split/difference views, zoom, and measurements.
- Make external test scenes easy to add, browse, and rerun for repeated AA comparisons.
- Use CMake for project generation.

## Non-goals

- Do not implement CMAA2 as an algorithm in this project.
- Do not tune or change Original SMAA to make it look better.
- Do not compare results from different render paths, color spaces, resolutions, or input frames.
- Do not introduce extra renderer features until the SMAA baseline is working.

## References

- Original SMAA: https://github.com/iryoku/smaa
- SMAA paper and project page: http://www.iryoku.com/smaa/
- CMAA2 sample: https://github.com/GameTechDev/CMAA2
- McGuire Computer Graphics Archive: https://casual-effects.com/data/

Reference boundary:

- `iryoku/smaa` is the algorithm baseline. Its `SMAA.hlsl`, `AreaTex`, and `SearchTex` assets should be treated as source-of-truth inputs.
- `GameTechDev/CMAA2` is only a demo and tooling reference. Useful ideas include static-image testing, AA mode switching, zoom inspection, reference capture, visual difference display, and benchmark panels.
- `casual-effects.com/data` is a scene/test-data reference. The demo should make downloaded scenes easy to register and test, but should not require large third-party scene packages to be committed to this repository.

## Proposed First Milestone

1. Create a minimal OpenGL app shell.
   Verify: window opens, full-screen triangle/quad renders a source texture.
2. Integrate Original SMAA in three passes.
   Verify: edge texture, blend-weight texture, and final output can each be displayed.
3. Add static-image loading.
   Verify: the same input image can be shown as `No AA` and `Original SMAA`.
4. Add comparison UI.
   Verify: mode selection, split view, zoom, and GPU timing are available.
5. Add scene-library registration.
   Verify: a downloaded scene folder can be added, listed in the UI, loaded, and reused for repeatable comparisons.

## Original SMAA Baseline Rules

- Vendor the upstream source with its license and record the upstream commit.
- Do not edit upstream SMAA shader logic in place.
- Put OpenGL-specific glue in wrapper files.
- Preserve SMAA pass order:
  1. edge detection
  2. blending weight calculation
  3. neighborhood blending
- Use the original precomputed area/search textures.
- Any change that affects Original SMAA output must be documented before it is made.

## Initial Technical Direction

- Language: C++17
- Build system: CMake
- Graphics API: OpenGL
- UI: Dear ImGui
- Initial input path: static image to texture
- Initial output modes: `No AA`, `Original SMAA`, `Original SMAA intermediates`
- Scene input path: local scene folders registered through a small manifest or UI import action

Current dependencies:

- GLFW for window/context creation
- Minimal in-repo OpenGL function loader
- Dear ImGui for UI
- stb_image and stb_image_write for image I/O
- tinyobjloader for the first OBJ scene-loading path

## Remaining Questions

These are still worth deciding after the first code pass:

- Dependency policy long-term: keep CMake `FetchContent`, vendor under `third_party/`, or rely on system packages?
- OpenGL target long-term: stay on 3.3 core for portability, or move to 4.5 core for cleaner debug/timing utilities?
- Original SMAA shader strategy long-term: keep `SMAA.hlsl` included through compatibility macros, or make a checked GLSL port from the upstream file?
- Test assets: include a small synthetic image set in-repo, or keep larger screenshots outside the repository?
- Scene import: OBJ is the first implemented path; glTF or broader Assimp support can be added later.
- Scene library storage: project-local `assets/scenes/`, user-local cache, or both?

## Current Implementation Snapshot

- CMake project skeleton with FetchContent dependencies.
- OpenGL 3.3 core window through GLFW and the in-repo OpenGL loader.
- Dear ImGui control panel.
- Generated static test pattern and user image loading.
- Original SMAA three-pass wrapper using upstream `SMAA.hlsl`, `AreaTex.h`, and `SearchTex.h`.
- Adaptive SMAA and Adaptive + TSCMAA SMAA placeholder modes. They currently run the same copied SMAA 1x pipeline and have separate shader directories for experiments.
- View modes for final, split, difference, edges, and blend weights.
- Scene manifest loading from `assets/scenes/scenes.json` or a local ignored manifest such as `assets/scenes/local.scenes.json`; the app prefers `local.scenes.json` when it exists.
- Scene entries load immediately when selected from the UI scene list.
- Input controls are source-specific: image controls are hidden in scene mode, and scene controls are hidden in image mode.
- OBJ scene rendering through `tinyobjloader`, including `vt` coordinates and `map_Kd` diffuse textures; the rendered scene color texture feeds the same SMAA comparison path as images.
- Scene camera controls include yaw, pitch, distance, target offset, FOV, exposure, reset, mouse orbit, and keyboard target movement.
