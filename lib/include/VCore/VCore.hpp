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

#ifndef VCore_HPP
#define VCore_HPP

// Public interface

// Export 
#include <VCore/Export/ExportSettings.hpp>
#include <VCore/Export/IExporter.hpp>
#include <VCore/Export/SpriteStackingExporter.hpp>

// Formats
#include <VCore/Formats/IVoxelFormat.hpp>
#include <VCore/Formats/SceneNode.hpp>

// Math
#include <VCore/Math/Mat4x4.hpp>
#include <VCore/Math/Vector.hpp>

// Meshing
#include <VCore/Meshing/Color.hpp>
#include <VCore/Meshing/IMesher.hpp>
#include <VCore/Meshing/Material.hpp>
#include <VCore/Meshing/Mesh.hpp>
#include <VCore/Meshing/MeshBuilder.hpp>
#include <VCore/Meshing/Texture.hpp>

// Miscellaneous
#include <VCore/Misc/Exceptions.hpp>
#include <VCore/Misc/FileStream.hpp>

// Voxel
#include <VCore/Voxel/BBox.hpp>
#include <VCore/Voxel/VoxelMesh.hpp>
#include <VCore/Voxel/PlanesVoxelizer.hpp>
#include <VCore/Voxel/VoxelTextureMap.hpp>
#include <VCore/Voxel/Frustum.hpp>

#endif //VCore_HPP