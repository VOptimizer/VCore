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

#ifndef MESH_HPP
#define MESH_HPP

#include <VoxelOptimizer/Math/Mat4x4.hpp>
#include <VoxelOptimizer/Meshing/Material.hpp>
#include <VoxelOptimizer/Formats/IVoxelFormat.hpp>
#include <memory>
#include <vector>
#include <VoxelOptimizer/Math/Vector.hpp>

namespace VoxelOptimizer
{
    struct SGroupedFaces
    {
        int MaterialIndex;             
        Material FaceMaterial;              //!< Material which are applied to this faces.
        std::vector<Math::Vec3f> Indices;       //!< Indices of the faces. 3 indices are always 1 triangle. One index is a tripple of x = vertex, y = normal, z = uv.
    };
    using GroupedFaces = std::shared_ptr<SGroupedFaces>;

    struct SMesh
    {
        std::vector<Math::Vec3f> Vertices;      //!< All vertices of this mesh.
        std::vector<Math::Vec3f> Normals;       //!< All normals of this mesh.
        std::vector<Math::Vec3f> UVs;           //!< All uvs of this mesh.

        std::vector<GroupedFaces> Faces;    //!< All faces of this mesh.
        std::map<TextureType, Texture> Textures;        //!< Texture used by this mesh.

        Math::Mat4x4 ModelMatrix; 
    };
    using Mesh = std::shared_ptr<SMesh>;
}


#endif //MESH_HPP