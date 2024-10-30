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
#include <iostream>

namespace VCore
{    
    const static uint32_t g_CMask = (CHUNK_SIZE - 1);
    const static uint32_t g_ChunkSizeP2 = (CHUNK_SIZE << 1);
    const static uint32_t g_Mask64 = ~(g_ChunkSizeP2 - 1);

    template<typename R>
    bool is_ready(std::future<R> const& f)
    { return f.wait_for(std::chrono::microseconds(0)) == std::future_status::ready; }

    inline Math::Vec3i GetChunkpos64(Math::Vec3i _Position, const Math::Vec3i &_Axis)
    {
        const static uint32_t mask = ~g_CMask;
        
        _Position.v[_Axis.x] &= mask;
        _Position.v[_Axis.y] &= g_Mask64;
        _Position.v[_Axis.z] &= mask;

        return _Position;
    }

    std::vector<SMeshChunk> CGreedyMesher::GenerateChunks(VoxelModel _Mesh, bool _OnlyDirty)
    {
        if(m_GenerateSingleChunks)
            return IMesher::GenerateChunks(_Mesh, _OnlyDirty);

        auto bbox = _Mesh->GetBBox();

        auto startTime = std::chrono::high_resolution_clock::now();

        std::chrono::milliseconds duration1(0);

        std::vector<std::future<Mesh>> futures;
        std::vector<Mesh> slices;
        for (int runAxis = 0; runAxis < 3; runAxis++)
        {
            auto begin = GetChunkpos(bbox.Beg).v[runAxis];
            auto end = GetChunkpos(bbox.End).v[runAxis] + CHUNK_SIZE;

            for (int axis = begin; axis < end; axis += CHUNK_SIZE)
            {
                futures.push_back(std::async(&CGreedyMesher::GenerateMeshSlices, this, _Mesh, bbox, runAxis, axis));
                auto startTime2 = std::chrono::high_resolution_clock::now();
                while(futures.size() >= std::thread::hardware_concurrency())
                {
                    auto it = futures.begin();
                    while (it != futures.end())
                    {
                        if(is_ready(*it))
                        {
                            auto result = it->get();
                            slices.push_back(std::move(result));              
                            it = futures.erase(it);
                        }
                        else
                            it++;
                    }
                }
                auto endTime2 = std::chrono::high_resolution_clock::now();
                duration1 += std::chrono::duration_cast<std::chrono::milliseconds>(endTime2 - startTime2);
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

        // for (auto &&ctx : taskContexts)
        //     delete ctx;

        auto endTime = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        // std::cout << "Thread time taken: " << duration.count() << " ms" << std::endl;

        // std::cout << "Thread time taken (Wait): " << duration1.count() << " ms" << std::endl;

        CMeshBuilder builder(m_SurfaceFactory);
        builder.AddTextures(_Mesh->Textures);

        Math::Vec3iHasher hasher;

        SMeshChunk chunk;
        chunk.UniqueId = hasher(bbox.Beg);
        chunk.InnerBBox = bbox;
        chunk.TotalBBox = bbox;
        chunk.MeshData = builder.Merge(nullptr, slices);

        // std::cout << "MaskgenerationTime taken: " << m_MaskgenerationTime.count() << " ms" << std::endl;
        // std::cout << "WidthgenerationTime taken: " << m_WidthgenerationTime.count() << " ms" << std::endl;

        m_MaskgenerationTime = std::chrono::milliseconds(0);
        m_WidthgenerationTime = std::chrono::milliseconds(0);

        return {chunk};
    }

    // Mesh CGreedyMesher::SlicerTask(TaskContext *_TaskContext)
    // {
    //     std::vector<Mesh> meshes;
    //     while (_TaskContext->NextSlice.load() < _TaskContext->ModelBBox.End.v[_TaskContext->Axis])
    //     {
    //         auto start = _TaskContext->NextSlice.load();
    //         _TaskContext->NextSlice += _TaskContext->SliceProcessCount;

    //         meshes.push_back(GenerateMeshSlices(*_TaskContext, start));
    //     }

    //     CMeshBuilder builder(m_SurfaceFactory);
    //     builder.AddTextures(_TaskContext->Model->Textures);

    //     return builder.Merge(nullptr, meshes);
    // }

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

    CFaceMask::Mask *CGreedyMesher::GetFaceMask(MeshSlicerContext &_Context, const Math::Vec3i &_Chunkpos, int d)
    {
        auto key = _Context.SliceIt->first;

        // Checks if the current chunk is already indexed.
        auto it = _Context.Chunks.find(_Chunkpos);
        if(it == _Context.Chunks.end())
        {
            CFaceMask maskGenerator;
            it = _Context.Chunks.insert({_Chunkpos, std::move(maskGenerator.Generate(_Context.Model, _Chunkpos, _Context.Axis.z))}).first;

            // Since the map changed, we need to optain the old iterators, so we can continue from the current position.
            _Context.DepthIt = _Context.Chunks[GetChunkpos64(_Context.Position, _Context.Axis)].find(d);
            _Context.SliceIt = _Context.DepthIt->second.find(key);
        }

        // Try to get the same slice with the same material properties as this one.
        auto depthIt = it->second.find(d);
        if(depthIt != it->second.end())
        {
            auto keyIt = depthIt->second.find(key);
            if(keyIt != depthIt->second.end())
                return &keyIt->second;
        }

        return nullptr;
    }

    BITMASK_TYPE *CGreedyMesher::GetFaces(MeshSlicerContext &_Context, const Math::Vec3i &_Chunkpos, int d, int x, bool _IsFront)
    {
        BITMASK_TYPE *faces = nullptr;
        auto mask = GetFaceMask(_Context, _Chunkpos, d);
        if(mask)
            faces = &mask->Bits[(x & g_CMask) + ((1 - (int)_IsFront) * CHUNK_SIZE)];

        return faces;     
    }

    void CGreedyMesher::GenerateMeshSlice(MeshSlicerContext &_Context, BITMASK_TYPE _Faces, bool _IsFront)
    {
        unsigned currentMaterial = -1;
        const int d = _Context.Position.v[_Context.Axis.z] & g_CMask;
        const int x = _Context.Position.v[_Context.Axis.x];
        int y = _Context.Position.v[_Context.Axis.y];
        
        while (y <= _Context.ModelBBox.End.v[_Context.Axis.y])
        {
            fast_vector<BITMASK_TYPE> bitmasks;
            BITMASK_TYPE heightPos = y & (g_ChunkSizeP2 - 1);
            BITMASK_TYPE totalHeight = 0;

            auto position = _Context.Position;
            position.v[_Context.Axis.y] = y;
            auto chunkpos = GetChunkpos64(position, _Context.Axis);

            auto startTime = std::chrono::high_resolution_clock::now();

            // Bitmask calculations
            while (true)
            {
                // Step 1: Get right y position
                auto zeros = CountTrailingZeroBits(_Faces >> heightPos);
                if(zeros == g_ChunkSizeP2)
                    zeros -= heightPos;

                // Only continues 1 bits can be grouped to one big mask!
                if((bitmasks.size() > 0) && zeros)
                    break;

                heightPos += zeros;

                // Add the zeros count to the global height position
                y += zeros; // Initial the zeros value could be greater than zero, after that it's always 0. (See if statement above)
                if(y > _Context.ModelBBox.End.v[_Context.Axis.y]) // Stop if we go over the model bounding box.
                    break;

                // Two chunks boundary reached
                if(heightPos >= g_ChunkSizeP2)
                {
                    // Resets the heightpos for the new chunk.
                    heightPos = 0;

                    // Step 2: Get the chunk group above this one.
                    chunkpos.v[_Context.Axis.y] += g_ChunkSizeP2;
                    if(chunkpos.v[_Context.Axis.y] > _Context.ModelBBox.End.v[_Context.Axis.y])
                        break;

                    auto faces = GetFaces(_Context, chunkpos, d, x, _IsFront);
                    if(!faces)
                        break;
                    
                    _Faces = *faces;
                }
                else
                {
                    // Step 3: Create the bitmask for the face groups.
                    BITMASK_TYPE faceCount = CountTrailingOneBits(_Faces >> heightPos);
                    BITMASK_TYPE mask = _Faces;
                    if(mask != UINT64_MAX)
                        mask = (((BITMASK_TYPE)1 << faceCount) - 1) << heightPos;

                    bitmasks.push_back(mask);
                    heightPos += faceCount;
                    totalHeight += faceCount;
                }
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            m_MaskgenerationTime += std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

            if(bitmasks.size() > 0)
            {
                // Step 4: Find the biggest face
                auto position = _Context.Position;

                startTime = std::chrono::high_resolution_clock::now();

                unsigned width = 1;
                position.v[_Context.Axis.y] = y;

                for (position.v[_Context.Axis.x] = x + 1; position.v[_Context.Axis.x] <= _Context.ModelBBox.End.v[_Context.Axis.x]; position.v[_Context.Axis.x]++)
                {
                    bool isContinues = false;

                    // Gets the current chunks mask
                    auto chunkpos = GetChunkpos64(position, _Context.Axis);
                    for (auto &&bitmask : bitmasks)
                    {
                        auto nextfaces = GetFaces(_Context, chunkpos, d, position.v[_Context.Axis.x], _IsFront);
                        if(!nextfaces)
                            break;

                        if((*nextfaces & bitmask) != bitmask)
                            break;

                        *nextfaces ^= bitmask;
                        isContinues = true;

                        chunkpos.v[_Context.Axis.y] += g_ChunkSizeP2;
                    }
                    
                    if(isContinues)
                        width++;
                    else
                        break;
                }

                endTime = std::chrono::high_resolution_clock::now();
                m_WidthgenerationTime += std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

                // Step 5: Build the mesh faces
                position.v[_Context.Axis.x] = x;
                position.v[_Context.Axis.y] = y;

                if(!_IsFront)
                    position.v[_Context.Axis.z] += 1;

                // auto chunkpos = GetChunkpos64(position, _Context.Axis);

                Math::Vec3f normal;
                normal.v[_Context.Axis.z] = _IsFront ? -1 : 1;

                Math::Vec3i size;
                size.v[_Context.Axis.z] = 0;
                size.v[_Context.Axis.y] = totalHeight;
                size.v[_Context.Axis.x] = width;

                Math::Vec3f du;
                du.v[_Context.Axis.x] = size.v[_Context.Axis.x];

                Math::Vec3f dv;
                dv.v[_Context.Axis.y] = size.v[_Context.Axis.y];

                auto key = _Context.SliceIt->first;
                Voxel voxel = (Voxel)&key;
                if((currentMaterial != voxel->Material) && (voxel->Material < (int)_Context.Model->Materials.size()))
                {
                    currentMaterial = voxel->Material;
                    _Context.Builder.SelectSurface(_Context.Model->Materials[voxel->Material]);
                }

                Math::Vec2f uv;
                if(_Context.Builder.GetTextures() && !_Context.Builder.GetTextures()->empty())
                    uv = Math::Vec2f(((float)(voxel->Color + 0.5f)) / _Context.Builder.GetTextures()->at(TextureType::DIFFIUSE)->GetSize().x, 0.5f);

                uint32_t idx1 = _Context.Builder.AddVertex(SVertex(position, normal, uv));
                uint32_t idx2 = _Context.Builder.AddVertex(SVertex(position + du, normal, uv));
                uint32_t idx3 = _Context.Builder.AddVertex(SVertex(position + dv, normal, uv));
                uint32_t idx4 = _Context.Builder.AddVertex(SVertex(position + size, normal, uv));

                if(_IsFront)
                    _Context.Builder.AddFace(idx1, idx2, idx3, idx4);
                else
                    _Context.Builder.AddFace(idx1, idx3, idx2, idx4);

                y += totalHeight;

                if((y & g_Mask64) > chunkpos.v[_Context.Axis.y])
                {
                    chunkpos.v[_Context.Axis.y] = y & g_Mask64;
                    auto faces = GetFaces(_Context, chunkpos, d, x, _IsFront);
                    if(!faces)
                        break;

                    _Faces = *faces;
                }
            }
            else
                break;
        }
    }

    Mesh CGreedyMesher::GenerateMeshSlices(const VoxelModel &_Model, const CBBox &_ModelBBox, int _RunAxis, int _AxisPos)
    {
        auto startTime = std::chrono::high_resolution_clock::now();

        MeshSlicerContext ctx(_Model, _ModelBBox, m_SurfaceFactory);

        // This logic calculates the index of one of the three other axis.
        // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
        // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y
        ctx.Axis = Math::Vec3i((_RunAxis + 2) % 3, (_RunAxis + 1) % 3, _RunAxis);

        CFaceMask mask;
        for (int x = _ModelBBox.Beg.v[ctx.Axis.x]; x <= _ModelBBox.End.v[ctx.Axis.x]; x++)
        {
            for (int d = _AxisPos; d < _AxisPos + CHUNK_SIZE; d++)              
            {
                ctx.Position.v[ctx.Axis.z] = d;
                ctx.Position.v[ctx.Axis.x] = x;
                ctx.Position.v[ctx.Axis.y] = _ModelBBox.Beg.v[ctx.Axis.y];

                while (true)
                {
                    // Gets the current chunks mask
                    auto chunkpos = GetChunkpos64(ctx.Position, ctx.Axis);
                    auto it = ctx.Chunks.find(chunkpos);
                    if(it == ctx.Chunks.end())
                        it = ctx.Chunks.insert({chunkpos, mask.Generate(_Model, chunkpos, _RunAxis)}).first;

                    // Checks if there is a slice for the current depth
                    ctx.DepthIt = it->second.find(d & g_CMask);
                    if(ctx.DepthIt != it->second.end())
                    {
                        ctx.SliceIt = ctx.DepthIt->second.begin();
                        while (ctx.SliceIt != ctx.DepthIt->second.end())
                        {
                            BITMASK_TYPE faces = ctx.SliceIt->second.Bits[x & g_CMask];
                            GenerateMeshSlice(ctx, faces, true);

                            faces = ctx.SliceIt->second.Bits[(x & g_CMask) + CHUNK_SIZE];
                            GenerateMeshSlice(ctx, faces, false);

                            ctx.SliceIt++;
                        }

                        break;
                    }
                    else
                    {
                        ctx.Position.v[ctx.Axis.y] += g_ChunkSizeP2;
                        if(ctx.Position.v[ctx.Axis.y] >= _ModelBBox.End.v[ctx.Axis.y])
                            break;
                    }
                }
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        // std::cout << "Slicer time taken: " << duration.count() << " ms" << std::endl;

        return ctx.Builder.Build();
    }
}
