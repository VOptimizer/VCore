# Kenshape file format

Kenshape files are just gzip compressed json files.

```json
{
    "title": "string",  // Name of the project
    "version": "string", // Version of Kenney Shape
    "size": {
        "x": "number",  // Canvas width
        "y": "number"   // Canvas size
    },
    "depthMultiplier": "float",   // Stretch multiplier around the z-axis.
    "alignment": "number",
    "tiles": [
        // Each pixel is an of object of the following format.
        {
            "shape": "number",  // Enum id of the type of the shape
            "angle": "number",  // Rotation angle of the shape
            "color": "number",  // Index of the color, which can be find in "colors"
            "depth": "number",  // Depth of the pixel goes up to 8
            "colorBack": "number",  // Color index of the back face
            "depthBack": "number"   // Depth of the back face. 0 means same size as depth.
        },
    ],
    // Contains the colorpalette of this model. Each color is html encoded. E.g.: "#000000" for black.
    "colors": [
    ]
}
```