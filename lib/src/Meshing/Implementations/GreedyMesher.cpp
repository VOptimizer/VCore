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

    inline Math::Vec3i GetChunkpos64(Math::Vec3i _Position, const Math::Vec3i &_Axis)
    {
        const static uint32_t mask = ~(CHUNK_SIZE - 1);
        const static uint32_t mask64 = ~((CHUNK_SIZE << 1) - 1);
        _Position.v[_Axis.x] &= mask;
        _Position.v[_Axis.y] &= mask64;
        _Position.v[_Axis.z] &= mask;

        return _Position;
    }

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
                while(futures.size() >= std::thread::hardware_concurrency())
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

    using MaskCollection = ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<uint32_t, CFaceMask::Mask>>;
    struct MeshSlicerContext
    {
        MeshSlicerContext(const VoxelModel &_Model, const CBBox &_ModelBBox, SurfaceFactory _Factory) : Model(_Model), ModelBBox(_ModelBBox), Builder(_Factory) 
        {
            Builder.AddTextures(Model->Textures);
        }   

        const VoxelModel &Model;
        CMeshBuilder Builder;
        const CBBox &ModelBBox;
        MaskCollection::iterator DepthIt;
        ankerl::unordered_dense::map<uint32_t, CFaceMask::Mask>::iterator SliceIt;
        Math::Vec3i Axis;
        Math::Vec3i Position;
        ankerl::unordered_dense::map<Math::Vec3i, MaskCollection, Math::Vec3iHasher> Chunks;
    };

    void GenerateMeshSlice1(MeshSlicerContext &_Context, BITMASK_TYPE _Faces, bool _IsFront)
    {
        unsigned currentMaterial = -1;
        int d = _Context.Position.v[_Context.Axis.z] & (CHUNK_SIZE - 1);
        int x = _Context.Position.v[_Context.Axis.x];
        int y = _Context.Position.v[_Context.Axis.y];

        auto key = _Context.SliceIt->first;
        
        CFaceMask maskGenerator;
        while (y <= _Context.ModelBBox.End.v[_Context.Axis.y])
        {
            fast_vector<BITMASK_TYPE> bitmasks;
            BITMASK_TYPE heightPos = y & ((CHUNK_SIZE << 1) - 1);
            BITMASK_TYPE totalHeight = 0;

            auto position = _Context.Position;
            position.v[_Context.Axis.y] = y;
            auto chunkpos = GetChunkpos64(position, _Context.Axis);
            auto it = _Context.Chunks.find(chunkpos);
            if(it == _Context.Chunks.end())
            {
                it = _Context.Chunks.insert({chunkpos, std::move(maskGenerator.Generate(_Context.Model, chunkpos, _Context.Axis.z))}).first;
                _Context.DepthIt = _Context.Chunks[GetChunkpos64(_Context.Position, _Context.Axis)].find(d);
                if(_Context.DepthIt == _Context.Chunks[GetChunkpos64(_Context.Position, _Context.Axis)].end())
                {
                    int i = 0;
                    i++;
                }

                _Context.SliceIt = _Context.DepthIt->second.find(key);
                if(_Context.SliceIt == _Context.DepthIt->second.end())
                {
                    int i = 0;
                    i++;
                }
            }

            auto depthIt = _Context.Chunks[chunkpos].find(d);
            if(depthIt != _Context.Chunks[chunkpos].end())
            {
                auto keyIt = depthIt->second.find(key);
                if(keyIt != depthIt->second.end())
                    _Faces = keyIt->second.Bits[(x & (CHUNK_SIZE - 1)) + ((1 - (int)_IsFront) * CHUNK_SIZE)];
                else
                    break;
            }
            else
                break;

            // _Faces = _Context.SliceIt->second.Bits[x & (CHUNK_SIZE - 1) + ((1 - (int)_IsFront) * CHUNK_SIZE)];

            // Bitmask calculations
            while (true)
            {
                // Step 1: Get right y position
                auto zeros = CountTrailingZeroBits(_Faces >> heightPos);
                if(zeros == (CHUNK_SIZE << 1))
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
                if(heightPos >= (CHUNK_SIZE * 2))
                {
                    // Resets the heightpos for the new chunk.
                    heightPos = 0;

                    // Step 2: Get the chunk group above this one.
                    chunkpos.v[_Context.Axis.y] += CHUNK_SIZE << 1;
                    if(chunkpos.v[_Context.Axis.y] > _Context.ModelBBox.End.v[_Context.Axis.y])
                        break;

                    auto it = _Context.Chunks.find(chunkpos);
                    if(it == _Context.Chunks.end())
                    {
                        it = _Context.Chunks.insert({chunkpos, std::move(maskGenerator.Generate(_Context.Model, chunkpos, _Context.Axis.z))}).first;
                        _Context.DepthIt = _Context.Chunks[GetChunkpos64(_Context.Position, _Context.Axis)].find(d);
                        if(_Context.DepthIt == _Context.Chunks[GetChunkpos64(_Context.Position, _Context.Axis)].end())
                        {
                            int i = 0;
                            i++;
                        }

                        _Context.SliceIt = _Context.DepthIt->second.find(key);
                        if(_Context.SliceIt == _Context.DepthIt->second.end())
                        {
                            int i = 0;
                            i++;
                        }
                    }

                    // Try to get the same slice with the same material properties as this one.
                    auto depthIt = it->second.find(d);
                    if(depthIt != it->second.end())
                    {
                        auto keyIt = depthIt->second.find(key);
                        if(keyIt != depthIt->second.end())
                            _Faces = keyIt->second.Bits[(x & (CHUNK_SIZE - 1)) + ((1 - (int)_IsFront) * CHUNK_SIZE)];
                        else
                            break;
                    }
                    else
                        break;
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

            if(bitmasks.size() > 0)
            {
                // Step 4: Find the biggest face
                auto position = _Context.Position;

                unsigned width = 1;
                for (int tmpWidth = x + 1; tmpWidth <= _Context.ModelBBox.End.v[_Context.Axis.x]; tmpWidth++)
                {
                    position.v[_Context.Axis.x] = tmpWidth;
                    position.v[_Context.Axis.y] = y;

                    bool isContinues = true;
                    for (auto &&bitmask : bitmasks)
                    {
                        isContinues = false;

                        // Gets the current chunks mask
                        auto chunkpos = GetChunkpos64(position, _Context.Axis);
                        auto it = _Context.Chunks.find(chunkpos);
                        if(it == _Context.Chunks.end())
                        {
                            it = _Context.Chunks.insert({chunkpos, std::move(maskGenerator.Generate(_Context.Model, chunkpos, _Context.Axis.z))}).first;
                            _Context.DepthIt = _Context.Chunks[GetChunkpos64(_Context.Position, _Context.Axis)].find(d);
                            if(_Context.DepthIt == _Context.Chunks[GetChunkpos64(_Context.Position, _Context.Axis)].end())
                            {
                                int i = 0;
                                i++;
                            }

                            _Context.SliceIt = _Context.DepthIt->second.find(key);
                            if(_Context.SliceIt == _Context.DepthIt->second.end())
                            {
                                int i = 0;
                                i++;
                            }
                        }

                        BITMASK_TYPE *nextfaces = 0;

                        // Try to get the same slice with the same material properties as this one.
                        auto depthIt = it->second.find(d);
                        if(depthIt != it->second.end())
                        {
                            auto keyIt = depthIt->second.find(key);
                            if(keyIt != depthIt->second.end())
                                nextfaces = &keyIt->second.Bits[(tmpWidth & (CHUNK_SIZE - 1)) + ((1 - (int)_IsFront) * CHUNK_SIZE)];
                            else
                                break;
                        }
                        else
                            break;

                        if((*nextfaces & bitmask) != bitmask)
                            break;

                        *nextfaces ^= bitmask;
                        isContinues = true;

                        position.v[_Context.Axis.y] += (CHUNK_SIZE << 1);
                    }
                    
                    if(isContinues)
                        width++;
                    else
                        break;
                }

                // Step 5: Build the mesh faces
                position.v[_Context.Axis.x] = x;
                position.v[_Context.Axis.y] = y;

                auto chunkpos = GetChunkpos64(position, _Context.Axis);

                Math::Vec3f normal;
                normal.v[_Context.Axis.z] = _IsFront ? -1 : 1;

                // Math::Vec3i position;
                position.v[_Context.Axis.z] = chunkpos.v[_Context.Axis.z] + d + (!_IsFront ? 1 : 0);
                position.v[_Context.Axis.y] = chunkpos.v[_Context.Axis.y] + (y & ((CHUNK_SIZE << 1) - 1));
                position.v[_Context.Axis.x] = chunkpos.v[_Context.Axis.x] + (x & (CHUNK_SIZE - 1));

                Math::Vec3i size;
                size.v[_Context.Axis.z] = 0;
                size.v[_Context.Axis.y] = totalHeight;
                size.v[_Context.Axis.x] = width;

                Math::Vec3f du;
                du.v[_Context.Axis.x] = size.v[_Context.Axis.x];

                Math::Vec3f dv;
                dv.v[_Context.Axis.y] = size.v[_Context.Axis.y];

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
            }
            else
                break;
        }
    }

    Mesh CGreedyMesher::GenerateMeshSlice(const VoxelModel &_Model, const CBBox &_ModelBBox, int _RunAxis, int _AxisPos)
    {
        MeshSlicerContext ctx(_Model, _ModelBBox, m_SurfaceFactory);

        // This logic calculates the index of one of the three other axis.
        // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
        // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y
        ctx.Axis = Math::Vec3i((_RunAxis + 2) % 3, (_RunAxis + 1) % 3, _RunAxis);

        CFaceMask mask;
        for (int d = _AxisPos; d < _AxisPos + CHUNK_SIZE; d++)
        {
            for (int x = _ModelBBox.Beg.v[ctx.Axis.x]; x <= _ModelBBox.End.v[ctx.Axis.x]; x++)
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
                        it = ctx.Chunks.insert({chunkpos, std::move(mask.Generate(_Model, chunkpos, _RunAxis))}).first;

                    // Checks if there is a slice for the current depth
                    ctx.DepthIt = it->second.find(d & (CHUNK_SIZE - 1));
                    if(ctx.DepthIt != it->second.end())
                    {
                        ctx.SliceIt = ctx.DepthIt->second.begin();
                        while (ctx.SliceIt != ctx.DepthIt->second.end())
                        {
                            // ctx.Position.v[ctx.Axis.x] = x;
                            // ctx.Position.v[ctx.Axis.y] = _ModelBBox.Beg.v[ctx.Axis.y];

                            BITMASK_TYPE faces = ctx.SliceIt->second.Bits[x & (CHUNK_SIZE - 1)];
                            GenerateMeshSlice1(ctx, faces, true);

                            // ctx.Position.v[ctx.Axis.x] = x;
                            // ctx.Position.v[ctx.Axis.y] = _ModelBBox.Beg.v[ctx.Axis.y];

                            int xpos = (x & (CHUNK_SIZE - 1));
                            faces = ctx.SliceIt->second.Bits[(x & (CHUNK_SIZE - 1)) + CHUNK_SIZE];
                            GenerateMeshSlice1(ctx, faces, false);

                            ctx.SliceIt++;
                        }

                        break;
                    }
                    else
                    {
                        ctx.Position.v[ctx.Axis.y] += CHUNK_SIZE << 1;
                        if(ctx.Position.v[ctx.Axis.y] >= _ModelBBox.End.v[ctx.Axis.y])
                            break;
                    }
                }
            }
                    


            // for (int h = _ModelBBox.Beg.v[heightAxis]; h < _ModelBBox.End.v[heightAxis]; h++)
            // {


            //     auto chunkpos = GetChunkpos(pos);
            //     auto it = chunks.find(chunkpos);
            //     if(it == chunks.end())
            //         it = chunks.insert({chunkpos, std::move(mask.Generate(_Model, pos, _RunAxis))}).first;

            //     auto depthIt = it->second.find(d);
            //     if(depthIt != it->second.end())
            //     {
            //         for (auto &&key : depthIt->second)
            //         {
            //             auto voxel = (Voxel)&key.first;

                    
                    
            //         }
            //     }
            // }

            // for (int w = _ModelBBox.Beg.v[widthAxis]; w < _ModelBBox.End.v[widthAxis]; w++)
            // {
            //     Math::Vec3i pos;
            //     pos.v[_RunAxis] = d;
            //     pos.v[widthAxis] = w;
            //     pos.v[heightAxis] = _ModelBBox.Beg.v[heightAxis];

            //     auto chunkpos = GetChunkpos(pos);
            //     auto it = chunks.find(chunkpos);
            //     if(it == chunks.end())
            //         it = chunks.insert({chunkpos, std::move(mask.Generate(_Model, pos, _RunAxis))}).first;
            //     pos.v[heightAxis] = chunkpos.v[heightAxis];

            //     int currentMaterial = -1;
            //     for (auto &&key : it->second[d])
            //     {
            //         auto voxel = (Voxel)&key.first;
            //         auto faces = key.second.Bits[w & (CHUNK_SIZE - 1)];

            //         int heightPos = 0;
            //         while (heightPos <= totalHeight)
            //         {
            //             // pos.v[heightAxis] = heightPos + _ModelBBox.Beg.v[heightAxis];

            //             heightPos += CountTrailingZeroBits(faces >> (heightPos & ((CHUNK_SIZE * 2) - 1)));
            //             if((heightPos & ((CHUNK_SIZE * 2) - 1)) >= ((CHUNK_SIZE * 2) - 1))
            //             {
            //                 pos.v[heightAxis] += (CHUNK_SIZE * 2);
            //                 chunkpos = GetChunkpos(pos);
            //                 it = chunks.find(chunkpos);
            //                 if(it == chunks.end())
            //                     it = chunks.insert({chunkpos, std::move(mask.Generate(_Model, pos, _RunAxis))}).first;
            //             }
            //                 // break;

            //             BITMASK_TYPE faceCount = CountTrailingOneBits(faces >> (heightPos & ((CHUNK_SIZE * 2) - 1)));
            //             BITMASK_TYPE mask = (((BITMASK_TYPE)1 << faceCount) - 1) << (heightPos & ((CHUNK_SIZE * 2) - 1));
            //         }
            //     }
            // }
        }

        
        {

            

            // int h = _ModelBBox.Beg.v[heightAxis];
            // while (h < _ModelBBox.End.v[heightAxis])
            // {
            //     Math::Vec3i pos;
            //     pos.v[_RunAxis] = _AxisPos;
            //     pos.v[heightAxis] = h;
            //     pos.v[widthAxis] = w;

            //     auto chunkpos = GetChunkpos(pos);
            //     chunks[chunkpos] = std::move(mask.Generate(_Model, pos, _RunAxis));
            //     h += CHUNK_SIZE * 2;
            // }
            

            // for (int h = _ModelBBox.Beg.v[heightAxis]; h < _ModelBBox.End.v[heightAxis]; h += CHUNK_SIZE * 2)
            // {
            //     Math::Vec3i pos;
            //     pos.v[_RunAxis] = _AxisPos;
            //     pos.v[heightAxis] = h;
            //     pos.v[widthAxis] = w;

            //     chunks[GetChunkpos(pos)] = std::move(mask.Generate(_Model, pos, _RunAxis));
            // }
        }

        return ctx.Builder.Build();
    }
}
