# Scene Library

Downloaded benchmark scenes should stay out of git by default. Add local scene entries to `scenes.json`.

Example:

```json
{
  "scenes": [
    {
      "name": "Bistro Exterior",
      "root": "D:/datasets/bistro",
      "asset": "Exterior/exterior.obj",
      "format": "obj",
      "default_camera": "main",
      "exposure": 1.0,
      "notes": "Downloaded from a local test dataset archive."
    }
  ]
}
```

The current implementation loads this registry in the UI and can render OBJ scenes into the same SMAA comparison path used by static images.
