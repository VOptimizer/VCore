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

#include <VCore/Math/Mat4x4.hpp>
#include <VCore/Meshing/Material.hpp>
#include <VCore/Formats/IVoxelFormat.hpp>
#include <memory>
#include <vector>
#include <VCore/Math/Vector.hpp>
#include <VCore/VConfig.hpp>

namespace VCore
{
    struct SVertex
    {
        SVertex() = default;
        SVertex(Math::Vec3f _Pos, Math::Vec3f _Normal, Math::Vec2f _UV) : Pos(_Pos), Normal(_Normal), UV(_UV) {}

        Math::Vec3f Pos;
        Math::Vec3f Normal;
        Math::Vec2f UV;

        inline bool operator==(const SVertex &_Vertex) const
        {
            return _Vertex.Pos == Pos && _Vertex.Normal == Normal && _Vertex.UV == UV;
        }
    };

    struct SSurface
    {
        SSurface() {}
        SSurface(SSurface &&_Other) { *this = std::move(_Other); }

        Material FaceMaterial;          //!< Material used for this group.

        // std::vector<SVertex> Vertices;  //!< Vertices of this surface

        VERTEX_DATA                     //!< Vertices of this surface
        ADD_VERTEX_DATA_METHOD
        GET_VERTEX_DATA_METHOD
        GET_VERTEX_STREAM_METHOD
        GET_VERTEX_DATA_SIZE_METHOD

        SSurface &operator=(SSurface &&_Other)
        {
            FaceMaterial = std::move(_Other.FaceMaterial);
            Indices = std::move(_Other.Indices);
            MOVE_VERTEX_DATA

            return *this;
        }

        std::vector<int> Indices;       //!< Indices for vertices used by this surface.
    };

    struct SMesh
    {
        std::vector<SSurface> Surfaces;                 //!< All surfaces of this mesh.
        std::map<TextureType, Texture> Textures;        //!< Texture used by this mesh.
        Math::Mat4x4 ModelMatrix;                       //!< Modelmatrix according to the voxel file.

        std::string Name;       //!< Same as of the voxel model.
        unsigned int FrameTime; //!< How long this frame should be last, in ms.
    };
    using Mesh = std::shared_ptr<SMesh>;

    struct VertexHasher
    {
        size_t operator()(const SVertex &_Vertex) const
        {
            Math::Vec3fHasher v3fhasher;
            Math::Vec2fHasher v2fhasher;

            size_t ph = v3fhasher(_Vertex.Pos);
            size_t nh = v3fhasher(_Vertex.Normal);
            size_t uvh = v2fhasher(_Vertex.UV);

            return ((ph * 73856093) ^ (nh * 19349663) ^ (uvh * 83492791));
        }
    };
}


#endif //MESH_HPP