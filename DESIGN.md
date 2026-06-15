# Design

This document defines the intended shape of the demo before implementation starts. The main constraint is measurement integrity: Original SMAA must remain a stable baseline so Improved SMAA can be compared against it.

## Assumptions

- The first usable demo should compare post-process AA on static images.
- A rendered 3D scene can be added after the image path is correct, but the design should reserve a clean scene-library path from the start.
- Original SMAA means the algorithm and lookup textures from `iryoku/smaa`.
- CMAA2 is a UI/tooling reference, not an implementation dependency.
- External scene archives, including scenes from the McGuire Computer Graphics Archive, should be user-added data rather than required repository contents.
- The user will build locally; this project should provide code and CMake, not require the agent to run builds.

## Success Criteria

- Original SMAA output can be rendered from a known input image.
- The app can switch between `No AA` and `Original SMAA` without changing input data.
- Intermediate SMAA textures can be inspected.
- GPU timing can be recorded for the SMAA passes.
- Downloaded test scenes can be registered, selected in the UI, and rendered through the same AA comparison path.
- Improved SMAA can later plug into the same interface without touching Original SMAA.

## Rendering Pipeline

Initial frame flow:

```text
source image or scene color
        |
        v
source color texture
        |
        +--> No AA output
        |
        +--> Original SMAA
              |
              +--> edge detection texture
              +--> blend weight texture
              +--> final SMAA texture
        |
        +--> Improved SMAA, later
              |
              +--> final improved texture
```

Original SMAA should be integrated as the standard three-pass path:

1. Edge detection
   - Use upstream functions such as `SMAALumaEdgeDetectionPS`, `SMAAColorEdgeDetectionPS`, or `SMAADepthEdgeDetectionPS`.
   - Write edge information to an intermediate edge texture.
2. Blending weight calculation
   - Use upstream `SMAABlendingWeightCalculationPS`.
   - Sample upstream area/search textures.
   - Write blending weights to an intermediate texture.
3. Neighborhood blending
   - Use upstream `SMAANeighborhoodBlendingPS`.
   - Combine the source color texture and blending weights into the final output texture.

The vertex helper functions from upstream SMAA should be preserved through wrapper shaders:

- `SMAAEdgeDetectionVS`
- `SMAABlendingWeightCalculationVS`
- `SMAANeighborhoodBlendingVS`

## Module Boundaries

Proposed source layout:

```text
CMakeLists.txt
cmake/
external/
  iryoku_smaa/
    SMAA.hlsl
    Textures/
    LICENSE.txt
src/
  app/
  gl/
  render/
  smaa_original/
  smaa_variant/
  smaa_improved/
  ui/
shaders/
  smaa_original/
  smaa_adaptive/
  smaa_adaptive_tscmaa/
assets/
  test_images/
  scenes/
```

Responsibilities:

- `app`: process lifetime, window, main loop, input dispatch.
- `gl`: OpenGL object wrappers, shader compilation, debug labels, timing queries.
- `render`: source image/scene rendering and post-process orchestration.
- `smaa_original`: immutable baseline integration wrappers around upstream SMAA.
- `smaa_variant`: copied SMAA 1x pipeline used by experimental variants.
- `smaa_improved`: experimental implementation with the same public interface.
- `ui`: ImGui controls for mode selection, inspection, and measurement.

## Scene Library

The demo should make testing real scenes less painful than the original sample apps. The scene feature is not a replacement for static-image testing; it is a second input source that feeds the same source color texture used by the SMAA comparison path.

Scene-library goals:

- Register a local scene folder without editing source code.
- List registered scenes in the UI.
- Store per-scene metadata such as display name, root path, main asset file, default camera, scale, and notes.
- Allow scene reload/rescan while developing.
- Keep large downloaded data out of git by default.
- Preserve repeatability by saving camera and render settings used for a comparison.

Proposed manifest shape:

```text
assets/scenes/scenes.json
```

Machine-specific downloaded scenes should use an ignored local manifest:

```text
assets/scenes/local.scenes.json
```

Each scene entry should describe:

- `name`
- `root`
- `asset`
- `format`
- `default_camera`
- `exposure`
- `notes`

Initial supported scene formats should stay narrow:

- static images first
- OBJ scenes first through `tinyobjloader`
- diffuse texture support through OBJ `vt` coordinates and MTL `map_Kd`
- glTF/GLB or broader import later through Assimp if needed
- additional formats only when a specific test scene requires them

The renderer should output one resolved scene color texture. SMAA should not know whether that texture came from an image or a 3D scene.

Current scene rendering is intentionally utilitarian. It supports material diffuse color and diffuse texture sampling for AA test visibility, but does not yet aim to reproduce archive reference renders with full lighting, normal maps, specular maps, or physically based materials.

Scene navigation should be practical enough for repeatable AA captures. The current camera is an orbit camera with target offsets, FOV, exposure, mouse orbit, and keyboard movement. Per-scene saved camera presets can be added after Sponza/Gallery framing is validated.

The UI should keep source-specific controls out of unrelated workflows. Image mode shows image loading controls only; scene mode shows scene selection, render size, and camera controls. Scene entries should load on selection, with a reload action only for explicitly refreshing the current scene.

## Original SMAA Isolation

Original SMAA should expose a small interface:

```text
OriginalSmaa::resize(width, height)
OriginalSmaa::execute(sourceColor, outputColor)
OriginalSmaa::debugTextures()
OriginalSmaa::lastTimings()
```

Rules:

- The upstream `SMAA.hlsl` file is not edited for experiments.
- If shader-language adaptation is required, use wrapper files and compile-time macros.
- Presets such as `SMAA_PRESET_LOW`, `SMAA_PRESET_MEDIUM`, `SMAA_PRESET_HIGH`, and `SMAA_PRESET_ULTRA` should map directly to upstream behavior.
- `SMAA_RT_METRICS` must match the active render target size every frame or resize.
- Area/search texture creation must be deterministic and shared only as read-only inputs.

## Improved SMAA Contract

Improved SMAA should not reuse mutable state from Original SMAA. It may share read-only source textures and test assets, but its shaders, buffers, and options should live separately.

Required contract:

- Same source color input.
- Same render target size.
- Same color space assumptions.
- Same viewport and sampler state unless a documented improvement requires otherwise.
- Comparable GPU timing scope.

Current experimental variants:

- `Adaptive SMAA`: placeholder copy of Original SMAA. Initial modification point is `shaders/smaa_adaptive/`.
- `Adaptive + TSCMAA SMAA`: placeholder copy of Original SMAA. Initial modification point is `shaders/smaa_adaptive_tscmaa/`.

Both variants must preserve the same three-pass output contract unless a design note explicitly changes it.

## UI and Measurement

Initial UI controls:

- AA mode: `No AA`, `Original SMAA`, `Improved SMAA` when available.
- Input mode: static image or scene.
- Scene selector: registered scene list, reload, and open/import action.
- View mode: final output, split view, difference view, intermediate textures.
- SMAA preset: low, medium, high, ultra.
- Edge input mode: luma first; color and depth can be added after baseline validation.
- Zoom tool: pixel-accurate magnified region.
- Benchmark controls: warmup count, sample count, median GPU time.

Metrics:

- GPU time per pass and total SMAA time.
- Optional CPU readback for image metrics after correctness is stable.
- PSNR/SSIM can be added later, but visual diff and exact image capture come first.

## Color and Sampling Rules

- Use a fixed render target format during comparisons.
- Keep source image upload, framebuffer output, and display conversion explicit.
- Do not compare an sRGB path against a linear path.
- Use identical sampler states between Original and Improved paths.
- Disable hidden post-processing in comparison views.

## Implementation Sequence

1. Static image viewer with OpenGL texture display.
2. Original SMAA wrapper shaders and lookup textures.
3. Intermediate texture inspection.
4. ImGui comparison controls.
5. Scene-library manifest and UI list.
6. Minimal OBJ scene loading/rendering path.
7. GPU timing and captured reference image comparison.
8. Improved SMAA skeleton with identical interface.

## Remaining Decisions

- Whether to stay on OpenGL 3.3 core or move to 4.5 core later.
- Whether to keep dependencies on CMake `FetchContent` or vendor them.
- Whether to keep upstream SMAA as copied source, move it to a submodule, or add an update script.
- Whether to keep OBJ-only loading for a while or add Assimp for glTF/other archive formats.
- Whether scene metadata should be a plain JSON file, an ImGui-managed registry, or both.
