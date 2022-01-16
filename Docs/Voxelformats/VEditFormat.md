# Overview

This document describes the container format of `.vedit` files. The current version of this format is the version 1.0.

# Structure

## Data types

The data types used in this document are defined as follows: 

| Name  | Size(Bytes)   | Description   |
|-------------- | -------------- | -------------- |
| any |  | Special type that can only be used as the `val_type` of a `dict`. See [Any type](#any-type) for more informations. |
| array | | An array of elements, the element count is stored in front of the array as `uint32`. |
| byte | 1 | Unsigned byte value. |
| charX | Depends on `X` | A fixed array of chars. |
| dict<val_type> | | An dictionary of key value pairs. See [Dict type](#dictionary-type) for more informations. |
| float | 4 | IEEE 754 32-Bit float |
| int32    | 4     | A 32-bit wide integer that is stored as little endian      |
| string | | A non-null terminated string. The size is stored before the string as int32. |
| uint32    | 4     | A 32-bit wide unsigned integer that is stored as little endian     |
| vec3 | 3 * float | A vector with 3 components. Stored in the order x, y, z. |

### Any type

The any type can only be used as `val_type` of a dictionary. The any has always an one byte type field which can be any value of the following table. This type byte is than followed by the type described above.

| Name   | Value    |
|--------------- | --------------- |
| STRING  | 0 |
| FLOAT  |  1 |
| INT32  | 2  |
| UINT32  | 3  |
| VECTOR3  | 4  |

### Dictionary type

A dictionary is a set of key-value pairs. The count of key-value pairs is stored in front of the dictionary as `uint32`. The `val_type` can be any data type that is described by this section. The keys are alwys unique `string` values.

## File header

Each file contains following header:

| Name  | Type   | Value   | Description |
|-------------- | -------------- | -------------- | -------------- |
| signature    | char5     | "VEDIT"     | Signature to classify the file. |
| version    | int32    | 0x1     | Version of the file. |
| program_version | char23 |  | Version of the program, that created this file |

## Sections

A file can contain one or more sections. Each section begins with the following header:

| Name  | Type   | Description   |
|-------------- | -------------- | -------------- |
| type    | int32     |  Section type.  |
| size | uint32 | Size in bytes of this section. |

### Types

| Name  | Value   | Description   |
|-------------- | -------------- | -------------- |
| META    | 0     | See [Meta section](#meta-section) for more information. |
| MATERIALS* | 1 | See [Materials section](#materials-section) for more information. |
| COLORPALETTE*    | 2     | See [Colorpalette section](#colorpalette-section) for more information. |
| VOXELS* | 3 | See [Voxels section](#voxels-section) for more information. |
| SCENETREE* | 4 | See [Scenetree section](#scenetree-section) for more information. |

`Note: All with '*' must follow in this order!`

All values up to and including 255 are reserved for the future. 

## Meta section

Currently undefined!

## Materials section

There can be multiple materials sections for multiple materials. Each material section contains one dictionary of type any. Following entries are defined:

| Name  | Type   | Description   |
|-------------- | -------------- | -------------- |
| name | string | Name of the material. Only used for gui. |
| metallic | float | Value between 0 and 1 |
| specular | float | Value between 0 and 1 |
| roughness | float | Value between 0 and 1 |
| ior | float | Value between 0 and 1 |
| power | float | Value between 0 and 1 |
| transparency | float | Value between 0 and 1 |

## Colorpalette section

| Name  | Type   | Description   |
|-------------- | -------------- | -------------- |
| name | string | Name of the color palette. |
| colors | array of byte | Zlib compressed data. |

### Zlib data

Array of following type:

| Name  | Type   | Description   |
|-------------- | -------------- | -------------- |
| red | byte | Red channel |
| green | byte | Green channel |
| blue | byte | Blue channel |
| alpha | byte | Alpha channel |

## Voxels section

| Name  | Type   | Description   |
|-------------- | -------------- | -------------- |
| properties | dict<any> | Dictionary of additional properties. |
| name | string | Name of the voxel mesh. Only used for gui. |
| thumbnail | array of byte | Embedded PNG file. Only used for gui. |
| reserved | uint32 | Reserved for the future. |
| size | vec3 | Size of the voxel space. This size can be larger than the voxel mesh itself. |
| data | array of byte | Zlib compressed voxel data. |

### Voxel data

The voxel data itself is an array of the following type:

| Name  | Type   | Description   |
|-------------- | -------------- | -------------- |
| position | vec3 | Position of the voxel. |
| material | uint32 | Index of the used material. |
| color | uint32 | Index of the used color. |
| reserved1 | uint32 | Reserved for the future |
| reserved2 | dict<any> | Reserved for the future |

## Scenetree section

| Name  | Type   | Description   |
|-------------- | -------------- | -------------- |
| name | string | Name of the node. |
| pivot | vec3 | Pivot of the mesh. |
| position | vec3 | Position of the mesh relative to its parent. |
| rotation | vec3 | Rotation of the mesh relative to its parent. The angles are stored as radians YXZ Euler angles. |
| scale | vec3 | Scale of the mesh relative to its parent. |
| mesh | uint32 | Index of the used mesh. |
| childrens | array of this type | Childrens of this node. | 