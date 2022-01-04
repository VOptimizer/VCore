# Overview

This documents describes all sections, which are written by the V-Edit tool.

# Types

This enum represents the `type` field of [Sections](VEditFormat.md#Sections).

| Name  | Value   | Description   |
|-------------- | -------------- | -------------- |
| COLORS    | 0     | See [Color section](#Color-section) for more information. |
| MATERIALS | 1 | See [Material section](#Material-section) for more information. |
| VOXELS | 2 | See [Voxel section](#Voxel-section) for more information. |

All values including up to 255 are reserved for the future. 

### Color section

This section contains the used colors.

| Name  | Type   | Description   |
|-------------- | -------------- | -------------- |
| color | byte4 | An array of R, G, B, A values. |

### Material section

This section contains the used materials. Each material is stored as `dict<any>`, following entries exists:

| Name  | Type   | Description   |
|-------------- | -------------- | -------------- |
| name | string | The name of the material |
| metallic | float | The metallic value of the material, in range of 0.0 - 1.0. |
| specular | float | The specular value of the material, in range of 0.0 - 1.0. |
| roughness | float | The roughness value of the material, in range of 0.0 - 1.0. |
| ior | float | The ior value of the material, in range of 0.0 - 1.0. |
| power | float | The power value of the material, in range of 0.0 - 1.0. |
| transparency | float | The transparency value of the material, in range of 0.0 - 1.0. |

### Voxel section

This section contains all the voxels. Each voxel is structured as follows:

| Name  | Type   | Description   |
|-------------- | -------------- | -------------- |
| size | vec3 | Size of the complete voxel space |
| data | array | Array of data |

#### Array data

| Name  | Type   | Description   |
|-------------- | -------------- | -------------- |
| pos | vec3 | Position of the voxel |
| mat_index | uint32 | The index of the material, which is used by this voxel |
| color_index | uint32 | The index of the color, which is used by this voxel |