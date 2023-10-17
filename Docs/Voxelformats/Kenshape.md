# Kenshape file format

Kenshape files are just gzip compressed json files.

```json
{
    "title": "string",  // Name of the project
    "author": "string",
    "size": {
        "x": "number",  // Canvas width
        "y": "number"   // Canvas size
    },
    "tiles": [
        // Each pixel is an of object of the following format.
        {
            "shape": "number",  // Enum id of the type of the shape
            "angle": "number",  // Rotation angle of the shape
            "color": "number",  // Index of the color, which can be find in "colors"
            "depth": "number",  // Depth of the pixel goes up to 8
            "enabled": "bool",
            "visited": "bool"
        },
    ],
    // Contains the colorpalette of this model. Each color is html encoded. E.g.: "#000000" for black.
    "colors": [
    ]
}
```