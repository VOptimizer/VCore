# Overview

This document describes the container format of `.vedit` files. The current version of this format is the version 1.0.

# Structure

## Data types

The data types used in this document are defined as follows: 

| Name  | Size(Bytes)   | Description   |
|-------------- | -------------- | -------------- |
| any |  | Special type that can only be used as the `val_type` of a `dict`. See [Any type](#any-type) for more informations. |
| array | | An array of elements, the element count is stored in front of the array as `uint32`. |
| byteX | Depends on `X` | A fixed array of bytes. Each byte is an unsigned value. |
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
| MATERIALS | 1 | See [Materials section](#materials-section) for more information. |
| COLORPALETTE    | 2     | See [Colorpalette section](#colorpalette-section) for more information. |
| VOXELS | 3 | See [Voxels section](#voxels-section) for more information. |
| SCENETREE | 4 | See [Scenetree section](#scenetree-section) for more information. |

All values up to and including 255 are reserved for the future. 

## Meta section

## Materials section

## Colorpalette section

## Voxels section

## Scenetree section