/*
 * MIT License
 *
 * Copyright (c) 2021 Christian Tost
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef VOXELOPTIMIZER_HPP
#define VOXELOPTIMIZER_HPP

/**
 * Public interface of the V-SDK 
 */

// Export 
#include <VoxelOptimizer/Export/ExportSettings.hpp>
#include <VoxelOptimizer/Export/IExporter.hpp>

// Formats
#include <VoxelOptimizer/Formats/IVoxelFormat.hpp>
#include <VoxelOptimizer/Formats/SceneNode.hpp>

// Math
#include <VoxelOptimizer/Math/Mat4x4.hpp>
#include <VoxelOptimizer/Math/Vector.hpp>

// Meshing
#include <VoxelOptimizer/Meshing/Color.hpp>
#include <VoxelOptimizer/Meshing/IMesher.hpp>
#include <VoxelOptimizer/Meshing/Material.hpp>
#include <VoxelOptimizer/Meshing/Mesh.hpp>
#include <VoxelOptimizer/Meshing/MeshBuilder.hpp>
#include <VoxelOptimizer/Meshing/Texture.hpp>
#include <VoxelOptimizer/Meshing/VerticesReducer.hpp>

// Miscellaneous
#include <VoxelOptimizer/Misc/Exceptions.hpp>

// Voxel
#include <VoxelOptimizer/Voxel/BBox.hpp>
#include <VoxelOptimizer/Voxel/VoxelMesh.hpp>
#include <VoxelOptimizer/Voxel/PlanesVoxelizer.hpp>

#endif //VOXELOPTIMIZER_HPP