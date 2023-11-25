# <img src="Docs/Assets/logo-vcore.png" alt="VCore" height="50">

VCore was originally developed for loading and optimizing various voxel formats. Evolving beyond its origins, VCore now focuses on real-time meshing, making it suitable for applications such as games.

## Disclaimer

Please be aware that, at the current stage of development, the public API may undergo changes at any time. Keep this in mind when using the library.

## Features

- Import of different [voxelformats](Docs/Voxelformats/README.MD)
- Multithreaded meshers with options including Simple, Greedy, and Marching Cubes
- Export capabilities to diverse 3D file formats such as Wavefront OBJ, GLTF, PLY, and Godot ESCN
- Voxel model export as sprite stacking images
- Basic frustum culling
- Animation support

## Planned features

- Occlusion culling
- Hardware-accelerated meshers (OpenCL, Cuda, Vulkan, OpenGL or ROCm)
- Terrain editing
- LOD mesh generation

## Getting started

### Prerequirements

Ensure you have the following prerequisites before getting started:

- [cmake](https://cmake.org/)
- C++ compiler supporting at least C++14 (Currently tested with gcc, compatibility with LLVM and MSVC expected)
- git (optional)
- scons (Only for the Godot build)

### Building static library

Follow these steps to build the library:

1. Clone this repo <br>
`git clone https://github.com/VOptimizer/VCore.git`

2. Build the library<br>
```bash
cd VCore/lib
mkdir build
cd build
cmake ..
cmake --build .
```
- Copy the static library and the `include` directory of the source tree to your project.

### Building gdnative

```bash
git clone --recursive https://github.com/godotengine/godot-cpp -b 3.5 # Call inside the root directory of vcore
cd godot-cpp
scons platform=<your platform> generate_bindings=yes
cd ..
cd gnative
mkdir build
cd build
cmake ..
cmake --build .
```

Replace `<your platform>` with either `windows`, `linux`, `osx` or `android`.

For more information please visit the official [repository](https://github.com/godotengine/godot-cpp/tree/3.5).

### Building CLI

```bash
cd cli
mkdir build
cd build
cmake ..
cmake --build .
```

The cli will be statically with VCore.

## Who is using VCore?

- [V-Optimizer](https://github.com/VOptimizer/VoxelOptimizer) on [itch.io](https://vailor1.itch.io/v-optimizer)

## Documentation

Explore the [examples](examples/) directory for detailed usage examples of this library or look at the [Sourcetree](Docs/Sourcetree/README.md) documentation for a deep dive into the source code.

For a more seamless integration of VCore into your engine or framework, additional information are available in the [Customization README](Docs/Customization/README.md).

## License

This library is under the [MIT License](LICENSE.txt)
