# Overview

This document describes the container format of `.vedit` files. The current version of this format is the version 1.0.

# Structure

## Data types

The data types used in this document are defined as follows: 

| Name  | Size(Bytes)   | Description   |
|-------------- | -------------- | -------------- |
| any | Depends of the type | Special type that can only be used as the `val_type` of a `dict`. The only purpose of this type is, to allow any data type in a `dict`, and is therefore just a placeholder. |
| byteX | Depends on `X` | A fixed array of bytes. Each byte is an unsigned value. |
| charX | Depends on `X` | A fixed array of chars. The `X` is the size of this array. This array is only nullterminated, if the string lenght is less than `X`. |
| dict<val_type> | | An dictionary of key value pairs. Where the keys are unique `string` values. The `val_type` can be any data type that is described by this section. The count of pairs is stored before the dict as int32. The size of the value is stored before the `value_type`. |
| float | 4 | IEEE 754 32-Bit float |
| int32    | 4     | A 32-Bit wide that which is stored as little endian     |
| uint32    | 4     | A 32-Bit wide unsigned that which is stored as little endian     |
| string | | A null terminated cstring. The size is stored before the string as int32. |
| vec3 | 3 * float | A vector with 3 components. Stored in the order x, y, z. |

## File header

Each file contains following header:

| Name  | Type   | Value   | Description |
|-------------- | -------------- | -------------- | -------------- |
| signature    | char4     | "VEDIT"     | Signature to classify the file. |
| version    | int32    | 0x1     | Version of the file. |
| program_version | char24 |  | Version of the program, that created this file |

## Sections

A file can contains one or more sections. Each section is described as follows:

| Name  | Type   | Description   |
|-------------- | -------------- | -------------- |
| type    | int32     | Classifies the section as for example materials sections. For more informations please visit the sections definitions.     |
| size | uint32 | Size in bytes of this section. |
