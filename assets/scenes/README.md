# Scene Library

Downloaded benchmark scenes should stay out of git by default.

- Keep `scenes.json` as the small checked-in registry.
- Use `local.scenes.json` for machine-specific absolute paths such as downloaded McGuire Computer Graphics Archive scenes.
- `local.scenes.json` is ignored by git through the `assets/scenes/*` rule.
- The app prefers `local.scenes.json` at startup when it exists.

Example:

```json
{
  "scenes": [
    {
      "name": "Crytek Sponza",
      "root": "C:/Users/USER/Desktop/sponza (1)",
      "asset": "sponza.obj",
      "format": "obj",
      "default_camera": "main",
      "exposure": 1.0,
      "notes": "McGuire Computer Graphics Archive, CC BY 3.0."
    }
  ]
}
```

The current implementation loads the manifest selected in the UI and can render textured OBJ scenes into the same SMAA comparison path used by static images.
