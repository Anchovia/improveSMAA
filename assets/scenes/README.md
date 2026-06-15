# Scene Library

Downloaded benchmark scenes should stay out of git by default unless they are explicitly curated as bundled test scenes.

- Keep `scenes.json` as the checked-in registry for bundled scenes.
- Curated bundled scenes live under `assets/scenes/vendor/`.
- Use `local.scenes.json` for machine-specific absolute paths such as downloaded McGuire Computer Graphics Archive scenes.
- `local.scenes.json` is ignored by git through the `assets/scenes/*` rule.
- The app loads `scenes.json` by default; use Advanced to select `local.scenes.json` manually when needed.

Example:

```json
{
  "scenes": [
    {
      "name": "Crytek Sponza",
      "root": "vendor/crytek_sponza",
      "asset": "sponza.obj",
      "format": "obj",
      "default_camera": "main",
      "exposure": 1.0,
      "notes": "McGuire Computer Graphics Archive, CC BY 3.0."
    }
  ]
}
```

The current implementation loads the manifest selected in Advanced, adds registered OBJ scenes to the main Source selector, and renders textured OBJ scenes into the same SMAA comparison path used by static images.

Bundled scenes:

- `vendor/crytek_sponza`: Crytek Sponza from the McGuire Computer Graphics Archive, CC BY 3.0.
- `vendor/gallery`: Gallery from the McGuire Computer Graphics Archive, CC BY-SA 4.0.
