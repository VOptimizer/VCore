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

#include <chrono>
#include <future>

#include "../../Misc/Helper.hpp"
#include <VCore/Meshing/Mesh/MeshBuilder.hpp>
#include <vector>

#include "GreedyMesher.hpp"

namespace VCore
{    
    template<typename R>
    bool is_ready(std::future<R> const& f)
    { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }

    std::vector<SMeshChunk> CGreedyMesher::GenerateChunks(VoxelModel _Mesh, bool _OnlyDirty)
    {
        if(m_GenerateSingleChunks)
            return IMesher::GenerateChunks(_Mesh, _OnlyDirty);

        auto bbox = _Mesh->GetBBox();

        std::vector<std::future<Mesh>> futures;
        std::vector<Mesh> slices;
        for (int runAxis = 0; runAxis < 3; runAxis++)
        {
            auto begin = GetChunkpos(bbox.Beg).v[runAxis];
            auto end = GetChunkpos(bbox.End).v[runAxis] + CHUNK_SIZE;

            for (int axis = begin; axis < end; axis += CHUNK_SIZE)
            {
                futures.push_back(std::async(&CGreedyMesher::GenerateMeshSlice, this, _Mesh, bbox, runAxis, axis));
                while(futures.size() >= 1) //std::thread::hardware_concurrency())
                {
                    auto it = futures.begin();
                    while (it != futures.end())
                    {
                        if(is_ready(*it))
                        {
                            auto result = it->get();
                            slices.push_back(result);              
                            it = futures.erase(it);
                        }
                        else
                            it++;
                    }
                }
            }
        }
        
        auto it = futures.begin();
        while (it != futures.end())
        {
            it->wait();
            auto result = it->get();
            slices.push_back(result);       
            it = futures.erase(it);
        }

        CMeshBuilder builder(m_SurfaceFactory);
        builder.AddTextures(_Mesh->Textures);

        Math::Vec3iHasher hasher;

        SMeshChunk chunk;
        chunk.UniqueId = hasher(bbox.Beg);
        chunk.InnerBBox = bbox;
        chunk.TotalBBox = bbox;
        chunk.MeshData = builder.Merge(nullptr, slices);

        return {chunk};
    }

    void CGreedyMesher::GenerateQuad(CMeshBuilder &result, const std::vector<Material> &_Materials, BITMASK_TYPE faces, CFaceMask::Mask &bits, int width, int depth, bool isFront, const Math::Vec3i &axis, const SChunkMeta &_Chunk, const Voxel _Voxel)
    {
        int currentMaterial = -1;

        BITMASK_TYPE heightPos = 0;
        // Shift werid = hang
        while ((heightPos <= (CHUNK_SIZE + 2)) && (faces >> heightPos))
        {
            heightPos += CountTrailingZeroBits(faces >> heightPos);
            if(heightPos >= CHUNK_SIZE)
                break;

            BITMASK_TYPE faceCount = CountTrailingOneBits(faces >> heightPos);
            BITMASK_TYPE mask = (((BITMASK_TYPE)1 << faceCount) - 1) << heightPos;

            unsigned w = 1;
            for (int tmpWidth = width + 1; tmpWidth < CHUNK_SIZE; tmpWidth++)
            {
                auto &nextfaces = bits.Bits[tmpWidth + ((1 - (int)isFront) * CHUNK_SIZE)];
                if((nextfaces & mask) != mask)
                    break;

                nextfaces ^= mask;
                w++;
            }

            Math::Vec3f normal;
            normal.v[axis.x] = isFront ? -1 : 1;

            Math::Vec3i position;
            position.v[axis.x] = _Chunk.TotalBBox.Beg.v[axis.x] + depth;
            position.v[axis.y] = _Chunk.TotalBBox.Beg.v[axis.y] + heightPos;
            position.v[axis.z] = _Chunk.TotalBBox.Beg.v[axis.z] + width;

            Math::Vec3i size;
            size.v[axis.x] = 0;
            size.v[axis.y] = faceCount;
            size.v[axis.z] = w;

            Math::Vec3f du;
            du.v[axis.z] = size.v[axis.z];

            Math::Vec3f dv;
            dv.v[axis.y] = size.v[axis.y];

            if((currentMaterial != _Voxel->Material) && (_Voxel->Material < (int)_Materials.size()))
            {
                currentMaterial = _Voxel->Material;
                result.SelectSurface(_Materials[_Voxel->Material]);
            }

            Math::Vec2f uv;
            if(result.GetTextures() && !result.GetTextures()->empty())
                uv = Math::Vec2f(((float)(_Voxel->Color + 0.5f)) / result.GetTextures()->at(TextureType::DIFFIUSE)->GetSize().x, 0.5f);

            uint32_t idx1 = result.AddVertex(SVertex(position, normal, uv));
            uint32_t idx2 = result.AddVertex(SVertex(position + du, normal, uv));
            uint32_t idx3 = result.AddVertex(SVertex(position + dv, normal, uv));
            uint32_t idx4 = result.AddVertex(SVertex(position + size, normal, uv));

            if(isFront)
                result.AddFace(idx1, idx2, idx3, idx4);
            else
                result.AddFace(idx1, idx3, idx2, idx4);

            heightPos += faceCount;
        }
    }

    SMeshChunk CGreedyMesher::GenerateMeshChunk(VoxelModel _Mesh, const SChunkMeta& _Chunk, bool)
    {
        CMeshBuilder builder(m_SurfaceFactory);
        builder.AddTextures(_Mesh->Textures);
        auto &materials = _Mesh->Materials;

        // For all 3 axis (x, y, z)
        for (size_t axis = 0; axis < 3; axis++)
        {
            // This logic calculates the index of one of the three other axis.
            int axis1 = (axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
            int axis2 = (axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y

            CFaceMask mask;
            auto masks = mask.Generate(_Mesh, _Chunk, axis);

            for (auto &&depth : masks)
            {
                for (auto &&key : depth.second)
                {
                    auto voxel = (Voxel)&key.first;

                    for (int widthAxis = 0; widthAxis < CHUNK_SIZE; widthAxis++)
                    {
                        auto faces = key.second.Bits[widthAxis];
                        GenerateQuad(builder, materials, faces, key.second, widthAxis, depth.first, true, Math::Vec3i(axis, axis1, axis2), _Chunk, voxel);

                        faces = key.second.Bits[widthAxis + CHUNK_SIZE];
                        GenerateQuad(builder, materials, faces, key.second, widthAxis, depth.first + 1, false, Math::Vec3i(axis, axis1, axis2), _Chunk, voxel);
                    }
                }
            }
        }

        SMeshChunk chunk;
        chunk.UniqueId = _Chunk.UniqueId;
        chunk.InnerBBox = _Chunk.InnerBBox;
        chunk.TotalBBox = _Chunk.TotalBBox;
        chunk.MeshData = builder.Build();

        return chunk;
    }

    Mesh CGreedyMesher::GenerateMeshSlice(const VoxelModel &_Model, const CBBox &_ModelBBox, int _RunAxis, int _AxisPos)
    {
        CMeshBuilder builder(m_SurfaceFactory);
        builder.AddTextures(_Model->Textures);

        // This logic calculates the index of one of the three other axis.
        int heightAxis = (_RunAxis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
        int widthAxis = (_RunAxis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y

        CFaceMask mask;
        // auto masks = mask.Generate(_Model, _Chunk, _RunAxis);

        return builder.Build();
    }
}
