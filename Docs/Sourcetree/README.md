# Sourcetree documentation

This document tries to give an overview of the source code structure. This hopefully helps to understand the source code and where thinks are located.

## Library

The VCore library is located inside the `lib` directory.

Inside the `include` directory lies the public interface of the library. Each directory inside the `ìnclude/VCore` directory represents a module. The idea behind this modular structure is, to ensure you could remove unnecessary modules during compile time.

The `src` directory mirrors the module structure of `ìnclude/VCore` and contains implementations for `meshers`, `exporters` and `voxel formats`. Also it provides some private api.

Last but not leas the `third_party` directory contains any used external library.

Diagram of the curren directory structure.
```
├───include
│   └───VCore
│       ├───Export
│       ├───Formats
│       ├───Math
│       ├───Meshing
│       ├───Misc
│       └───Voxel
├───src
│   ├───Export
│   │   └───Implementations
│   │       └───glTF
│   ├───Formats
│   │   └───Implementations
│   │       └───Qubicle
│   ├───Meshing
│   │   └───Implementations
│   │       └───Slicer
│   ├───Misc
│   └───Voxel
└───third_party
```

## CLI

Inside the `cli` directory you find the source code for the cli program. This could be used as demonstration of how to use the library, but the examples inside `examples` are maybe easier to understand.

## gdnative

The `gdnative` directory contains the implementation for Godot Native library. This could be used as demonstration of how to use the library, but the examples inside `examples` are maybe easier to understand.

## How to implement a new X?

- [Exporter](Exporter.md)
- [Mesher](Mesher.md)
- [VoxelFormat](VoxelFormat.md)