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
#include <cstdint>
#include <future>

#include "../../Misc/Helper.hpp"
#include "../../Misc/TexturePacker.hpp"
#include <VCore/Meshing/Mesh/MeshBuilder.hpp>
#include <VCore/Meshing/MaterialManager.hpp>
#include <vector>

#include "GreedyMesher.hpp"
#include "../../Simd/Simd.hpp"
#include "VCore/Voxel/Voxel.hpp"

namespace VCore
{
    template<typename R>
    bool is_ready(std::future<R> const& f)
    { return f.wait_for(std::chrono::microseconds(0)) == std::future_status::ready; }

    // std::vector<SMeshChunk> CGreedyMesher::GenerateChunks(VoxelModel _Mesh, bool _OnlyDirty)
    // {
    //     if(m_GenerateSingleChunks)
    //         return IMesher::GenerateChunks(_Mesh, _OnlyDirty);

    //     auto bbox = _Mesh->CalculateBBox();

    //     std::vector<std::future<Mesh>> futures;
    //     std::vector<Mesh> slices;
    //     for (int runAxis = 0; runAxis < 3; runAxis++)
    //     {
    //         auto begin = GetChunkpos(bbox.Beg).v[runAxis];
    //         auto end = GetChunkpos(bbox.End).v[runAxis] + Config::ChunkSize;

    //         for (int axis = begin; axis < static_cast<int>(end); axis += Config::ChunkSize)
    //         {
    //             futures.push_back(std::async(&CGreedyMesher::GenerateMeshSlices, this, _Mesh, bbox, runAxis, axis));
    //             while(futures.size() >= std::thread::hardware_concurrency())
    //             {
    //                 auto it = futures.begin();
    //                 while (it != futures.end())
    //                 {
    //                     if(is_ready(*it))
    //                     {
    //                         auto result = it->get();
    //                         slices.push_back(std::move(result));              
    //                         it = futures.erase(it);
    //                     }
    //                     else
    //                         it++;
    //                 }
    //             }
    //         }
    //     }
        
    //     auto it = futures.begin();
    //     while (it != futures.end())
    //     {
    //         it->wait();
    //         auto result = it->get();
    //         slices.push_back(result);       
    //         it = futures.erase(it);
    //     }

    //     CMeshBuilder builder(m_SurfaceFactory);
    //     builder.AddTextures(_Mesh->Textures);

    //     Math::Vec3iHasher hasher;

    //     SMeshChunk chunk;
    //     chunk.UniqueId = hasher(bbox.Beg);
    //     chunk.InnerBBox = bbox;
    //     chunk.TotalBBox = bbox;
    //     chunk.MeshData = builder.Merge(nullptr, slices);

    //     if(m_GenerateTexture)
    //     {
    //         TTexturePacker<TextureInfo> packer;
    //         auto head = m_Head.load();
    //         while (head)
    //         {
    //             packer.AddRect(head->Info.Size, std::move(head->Info));
    //             head = head->Next;
    //         }
            
    //         auto rects = packer.Pack();
    //         ankerl::unordered_dense::map<uint32_t, decltype(rects)::value_type> textureMapping;
    //         auto texture = std::make_shared<CTexture>(packer.GetCanvasSize());

    //         for (auto &&rect : rects)
    //         {
    //             textureMapping[rect.Reference.Id] = rect;
    //             CopyToAtlas(texture, _Mesh, rect.Position, rect.Reference);


    //             // texture->AddRawPixels(rect.Reference.RawTexture, rect.Position, rect.Size);
    //         }

    //         chunk.MeshData->Textures[TextureType::DIFFIUSE] = texture;
            
    //         for (auto &&surface : chunk.MeshData->Surfaces)
    //         {
    //             for (uint32_t i = 0; i < surface->GetVertexCount(); i += 4)
    //             {
    //                 auto v1 = surface->GetVertex(i);
    //                 auto v2 = surface->GetVertex(i + 1);
    //                 auto v3 = surface->GetVertex(i + 2);
    //                 auto v4 = surface->GetVertex(i + 3);
    //                 // TODO: Currently no UVs! How to map Texture?
    //                 // auto textureRect = textureMapping[v1.UV.x];

    //                 // For each position we need to substract the margin pixels.

    //                 // v1.UV = Math::Vec2f(textureRect.Position.x + 1, textureRect.Position.y + 1) / packer.GetCanvasSize();
    //                 // v2.UV = Math::Vec2f((textureRect.Position.x + 1) + (textureRect.Size.x - 2), textureRect.Position.y + 1) / packer.GetCanvasSize();
    //                 // v3.UV = Math::Vec2f(textureRect.Position.x + 1, (textureRect.Position.y + 1) + (textureRect.Size.y - 2)) / packer.GetCanvasSize();
    //                 // v4.UV = Math::Vec2f((textureRect.Position + Math::Vec2f(1, 1)) + (textureRect.Size - Math::Vec2f(2, 2))) / packer.GetCanvasSize();

    //                 surface->UpdateVertex(i, v1);
    //                 surface->UpdateVertex(i + 1, v2);
    //                 surface->UpdateVertex(i + 2, v3);
    //                 surface->UpdateVertex(i + 3, v4);
    //             }
    //         }

    //         ClearTextures();
    //     }

    //     return {chunk};
    // }

    void CGreedyMesher::CopyToAtlas(Texture &p_Atlas, const VoxelModel &p_Model, const Math::Vec2ui &p_Position, const TextureInfo &p_Info)
    {
        // auto diffuse = p_Model->Textures.find(TextureType::DIFFIUSE);
        // if(diffuse != p_Model->Textures.end())
        // {
        //     auto size = p_Info.Size - Math::Vec2ui(2, 2);

        //     for (uint32_t x = 0; x < size.x; x++)
        //     {
        //         for (uint32_t y = 0; y < size.y; y++)
        //         {
        //             auto pos = p_Info.Position;
        //             pos.v[p_Info.Axis.x] += x;
        //             pos.v[p_Info.Axis.y] += y;

        //             auto voxIt = p_Model->find(pos);
        //             if(voxIt != p_Model->end())
        //             {
        //                 auto pixel = diffuse->second->GetPixel(Math::Vec2ui(voxIt->second.GetColor(), 0));
        //                 p_Atlas->AddPixel(pixel, p_Position + Math::Vec2ui(x + 1, y + 1));

        //                 // Adds margin pixels to the texture
        //                 if((x == 0) && (y == 0))
        //                     p_Atlas->AddPixel(pixel, p_Position + Math::Vec2ui(x, y));

        //                 if(x == 0)
        //                     p_Atlas->AddPixel(pixel, p_Position + Math::Vec2ui(x, y + 1));

        //                 if(y == 0)
        //                     p_Atlas->AddPixel(pixel, p_Position + Math::Vec2ui(x + 1, y));
                        
        //                 if(((x + 1) == size.x) && ((y + 1) == size.y))
        //                     p_Atlas->AddPixel(pixel, p_Position + Math::Vec2ui(x + 2, y + 2));

        //                 if((x + 1) == size.x)
        //                     p_Atlas->AddPixel(pixel, p_Position + Math::Vec2ui(x + 2, y + 1));

        //                 if((y + 1) == size.y)
        //                     p_Atlas->AddPixel(pixel, p_Position + Math::Vec2ui(x + 1, y + 2));

        //                 // Top right
        //                 if(((x + 1) == size.x) && (y == 0))
        //                     p_Atlas->AddPixel(pixel, p_Position + Math::Vec2ui(x + 2, y));

        //                 // Bottom left
        //                 if((x == 0) && ((y + 1) == size.y))
        //                     p_Atlas->AddPixel(pixel, p_Position + Math::Vec2ui(x, y + 2));
        //             }
        //         }
        //     }
        // }
    }

    void CGreedyMesher::GenerateQuad(CMeshBuilder &p_Result, Config::bitmask_t p_Faces, CFaceMask::Mask &p_Bits, int p_Width, int p_Depth, bool p_IsFront, const Math::Vec3i &p_Origin, const Math::Vec3i &p_Axis, const SChunkMeta &p_Chunk, const CVoxel& p_Voxel, uint8_t p_Ao)
    {
        uint8_t currentMaterial = 0xFF;
        auto voxmaterial = p_Voxel.GetMaterial();
        auto color = p_Voxel.GetColor();

        // bool hasTexture = result.GetTextures() && !result.GetTextures()->empty();
        // int textureWidth = 0;
        // if(hasTexture)
        //     textureWidth = result.GetTextures()->at(TextureType::DIFFIUSE)->GetSize().x;

        Config::bitmask_t heightPos = 0;
        // Shift werid = hang
        while ((heightPos < Config::ChunkSize) && (p_Faces >> heightPos))
        {
            // ~ 5ms
            heightPos += CountTrailingZeroBits(p_Faces >> heightPos);
            if(heightPos >= Config::ChunkSize)
                break;

            // Math::Vec3i position;
            // position.v[axis.x] = depth - 1;
            // position.v[axis.y] = 0;
            // position.v[axis.z] = width - 1;

            Config::bitmask_t faceCount = CountTrailingOneBits(p_Faces >> heightPos);
            // if(position.v[axis.x] > 0 && position.v[axis.z] > 0)
            // {
            //     auto voxels = _Chunk.Chunk->m_Mask.GetRowFaces(position, axis.x) >> heightPos;
            //     auto voxelCount = CountTrailingOneBits(voxels);
            //     if(voxelCount != 0 && voxelCount != faceCount)
            //         faceCount = voxelCount;
            // }



            Config::bitmask_t mask = Config::BitmaskMax;
            if(faceCount != Config::ChunkSize)
                mask = (((Config::bitmask_t)1 << faceCount) - 1) << heightPos;

            Simd::NativeI simd_mask(mask);
            const int integers = sizeof(Simd::NativeI) / sizeof(int);

            unsigned w = 1;
            for (uint32_t tmpWidth = p_Width + 1; tmpWidth < Config::ChunkSize; tmpWidth += integers)
            {
                const size_t rounds = (tmpWidth + integers < Config::ChunkSize) ? integers : (Config::ChunkSize - tmpWidth);
                int smask = (1 << rounds) - 1;

                // int buf[integers] = {};
                // for (size_t i = 0; i < rounds; i++)
                //     buf[i] = bits.Bits[(tmpWidth + i) + ((1 - (int)isFront) * Config::ChunkSize)];

                Simd::NativeI simd_cols((int*)&p_Bits.Bits[tmpWidth + ((1 - (int)p_IsFront) * Config::ChunkSize)], integers);

                auto result = (simd_mask & simd_cols) == simd_mask;
                int mResult = result.MoveMask();
                auto colcnt = CountTrailingOneBits(mResult & smask);

                for (size_t i = 0; i < colcnt; i++)
                    p_Bits.Bits[(tmpWidth + i) + ((1 - (int)p_IsFront) * Config::ChunkSize)] ^= mask;
                    
                w += colcnt;
                if(colcnt != rounds)
                    break;
            }

            // ~40ms

            Math::Vec3f normal;
            normal.v[p_Axis.x] = p_IsFront ? -1 : 1;

            Math::Vec3i position;
            position.v[p_Axis.x] = p_Chunk.TotalBBox.Beg.v[p_Axis.x] + p_Depth;
            position.v[p_Axis.y] = p_Chunk.TotalBBox.Beg.v[p_Axis.y] + heightPos;
            position.v[p_Axis.z] = p_Chunk.TotalBBox.Beg.v[p_Axis.z] + p_Width;

            Math::Vec3i size;
            size.v[p_Axis.x] = 0;
            size.v[p_Axis.y] = faceCount;
            size.v[p_Axis.z] = w;

            Math::Vec3f rightDirection;
            rightDirection.v[p_Axis.z] = size.v[p_Axis.z];

            Math::Vec3f upDirection;
            upDirection.v[p_Axis.y] = size.v[p_Axis.y];

            if(currentMaterial != voxmaterial)
            {
                currentMaterial = voxmaterial;
                p_Result.SelectSurface(voxmaterial);
            }

            // Math::Vec2f uv;
            // if(hasTexture)
            //     uv = Math::Vec2f(((float)(_Voxel.Color + 0.5f)) / textureWidth, 0.5f);

            uint8_t ao1 = p_Ao & 3;
            uint8_t ao2 = (p_Ao >> 2) & 3;
            uint8_t ao3 = (p_Ao >> 4) & 3;
            uint8_t ao4 = (p_Ao >> 6) & 3;

            bool needFlip = ao2 + ao3 < ao1 + ao4;

            uint32_t idx1 = p_Result.AddVertex(new SVertex(position - p_Origin, normal, color, ao1));
            uint32_t idx2 = p_Result.AddVertex(new SVertex((position + rightDirection) - p_Origin, normal, color, ao2));
            uint32_t idx3 = p_Result.AddVertex(new SVertex((position + upDirection) - p_Origin, normal, color, ao3));
            uint32_t idx4 = p_Result.AddVertex(new SVertex((position + size) - p_Origin, normal, color, ao4));

            if(p_IsFront)
            {
                if(!needFlip)
                    p_Result.AddFace(idx1, idx2, idx3, idx4);
                else
                {
                    p_Result.AddFace(idx1, idx4, idx3);
                    p_Result.AddFace(idx1, idx2, idx4);
                }
            }
            else
            {
                if(!needFlip)
                    p_Result.AddFace(idx1, idx3, idx2, idx4);
                else
                {
                    p_Result.AddFace(idx1, idx4, idx2);
                    p_Result.AddFace(idx1, idx3, idx4);

                }
            }

            heightPos += faceCount;
        }
    }

    uint32_t CGreedyMesher::AddTexture(const Math::Vec3i &p_Position, const Math::Vec3i &p_Axis, const Math::Vec2ui &p_Size)
    {
        // std::lock_guard<std::mutex> lock(m_Lock);

        uint32_t id = m_NextId++;

        // auto node = new TextureNode(id, _Position, _Axis, _Size);
        TextureNode *node = m_Pool.construct(id, p_Position, p_Axis, p_Size);
        auto head = m_Head.load(std::memory_order_relaxed);
        do
        {
            node->Next = head;
        } while(!m_Head.compare_exchange_weak(head, node, std::memory_order_release, std::memory_order_relaxed));

        return id;
    }

    void CGreedyMesher::ClearTextures()
    {
        if(m_Head)
        {
            while (m_Head)
            {
                auto next = m_Head.load()->Next;
                // delete m_Head.load();
                m_Pool.destruct(m_Head.load());
                m_Head = next;
            }
        }
    }

    SMeshChunk CGreedyMesher::GenerateMeshChunk(VoxelModel p_Mesh, const SChunkMeta& p_Chunk, bool)
    {
        CMeshBuilder builder(m_SurfaceFactory);
        // builder.AddTextures(p_Mesh->Textures);

        // For all 3 axis (x, y, z)
        for (size_t axis = 0; axis < 3; axis++)
        {
            // This logic calculates the index of one of the three other axis.
            int axis1 = (axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
            int axis2 = (axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y

            CFaceMask mask;

            // ~15ms

            // ~4ms without Grouping
            // ~10ms with grouping
            auto masks = mask.Generate(p_Mesh, p_Chunk, axis);
            
            for (auto &&depth : masks)
            {
                for (auto &&key : depth.second)
                {
                    auto voxel = CVoxel(key.first & 0xFFFFFFFF);
                    auto ao = (key.first >> 32);

                    for (uint32_t widthAxis = 0; widthAxis < Config::ChunkSize; widthAxis++)
                    {
                        auto faces = key.second.Bits[widthAxis];

                        if(faces)
                            GenerateQuad(builder, faces, key.second, widthAxis, depth.first, true, p_Mesh->Origin, Math::Vec3i(axis, axis1, axis2), p_Chunk, voxel, ao);

                        faces = key.second.Bits[widthAxis + Config::ChunkSize];

                        if(faces)
                            GenerateQuad(builder, faces, key.second, widthAxis, depth.first + 1, false, p_Mesh->Origin, Math::Vec3i(axis, axis1, axis2), p_Chunk, voxel, ao);
                    }
                }
            }
        }

        SMeshChunk chunk;
        chunk.UniqueId = p_Chunk.UniqueId;
        chunk.InnerBBox = p_Chunk.InnerBBox;
        chunk.TotalBBox = p_Chunk.TotalBBox;
        chunk.MeshData = builder.Build();

        return chunk;
    }

    CFaceMask::Mask *CGreedyMesher::GetFaceMask(MeshSlicerContext &p_Context, const Math::Vec3i &p_Chunkpos, int p_Depth)
    {
        auto key = p_Context.SliceIt->first;

        // Checks if the current chunk is already indexed.
        auto it = p_Context.Chunks.find(p_Chunkpos);
        if(it == p_Context.Chunks.end())
        {
            CFaceMask maskGenerator;
            maskGenerator.GroupAfterMaterial = m_GenerateTexture;
            it = p_Context.Chunks.insert({p_Chunkpos, maskGenerator.Generate(p_Context.Model, p_Chunkpos, p_Context.Axis.z)}).first;

            // Since the map changed, we need to optain the old iterators, so we can continue from the current position.
            p_Context.DepthIt = p_Context.Chunks[GetChunkpos(p_Context.Position)].find(p_Depth);
            p_Context.SliceIt = p_Context.DepthIt->second.find(key);
        }

        // Try to get the same slice with the same material properties as this one.
        auto depthIt = it->second.find(p_Depth);
        if(depthIt != it->second.end())
        {
            auto keyIt = depthIt->second.find(key);
            if(keyIt != depthIt->second.end())
                return &keyIt->second;
        }

        return nullptr;
    }

    Config::bitmask_t *CGreedyMesher::GetFaces(MeshSlicerContext &p_Context, const Math::Vec3i &p_Chunkpos, int p_Depth, int p_XPos, bool p_IsFront)
    {
        Config::bitmask_t *faces = nullptr;
        auto mask = GetFaceMask(p_Context, p_Chunkpos, p_Depth);
        if(mask)
            faces = &mask->Bits[(p_XPos & Config::InnerChunkMask) + ((1 - (int)p_IsFront) * Config::ChunkSize)];

        return faces;     
    }

    void CGreedyMesher::GenerateMeshSlice(MeshSlicerContext &p_Context, Config::bitmask_t p_Faces, bool p_IsFront)
    {
        uint8_t currentMaterial = 0xFF;
        const int d = p_Context.Position.v[p_Context.Axis.z] & Config::InnerChunkMask;
        const int x = p_Context.Position.v[p_Context.Axis.x];
        int y = p_Context.Position.v[p_Context.Axis.y];
        
        while (y <= p_Context.ModelBBox.End.v[p_Context.Axis.y])
        {
            fast_vector<Config::bitmask_t> bitmasks;
            Config::bitmask_t heightPos = y & Config::InnerChunkMask;
            Config::bitmask_t totalHeight = 0;

            auto position = p_Context.Position;
            position.v[p_Context.Axis.y] = y;
            auto chunkpos = GetChunkpos(position);

            // Bitmask calculations
            while (true)
            {
                // Step 1: Get right y position
                auto zeros = heightPos >= Config::ChunkSize ? 0 : CountTrailingZeroBits(p_Faces >> heightPos);
                if(zeros == Config::ChunkSize)
                    zeros -= heightPos;

                // Only continues 1 bits can be grouped to one big mask!
                if((bitmasks.size() > 0) && zeros)
                    break;

                heightPos += zeros;

                // Add the zeros count to the global height position
                y += zeros; // Initial the zeros value could be greater than zero, after that it's always 0. (See if statement above)
                if(y > p_Context.ModelBBox.End.v[p_Context.Axis.y]) // Stop if we go over the model bounding box.
                    break;

                // Two chunks boundary reached
                if(heightPos >= Config::ChunkSize)
                {
                    // Resets the heightpos for the new chunk.
                    heightPos = 0;

                    // Step 2: Get the chunk group above this one.
                    Config::bitmask_t *faces = nullptr;

                    // Loops until the next chunk is reached or no more chunks follows the current one.
                    while (!faces)
                    {
                        chunkpos.v[p_Context.Axis.y] += Config::ChunkSize;
                        if(chunkpos.v[p_Context.Axis.y] > p_Context.ModelBBox.End.v[p_Context.Axis.y])
                        {
                            chunkpos.v[p_Context.Axis.y] -= Config::ChunkSize;
                            break;
                        }

                        faces = GetFaces(p_Context, chunkpos, d, x, p_IsFront);

                        // Breaks the loop immediately, if no chunk follows the current one.
                        if(!faces && bitmasks.size() > 0)
                        {
                            chunkpos.v[p_Context.Axis.y] -= Config::ChunkSize;
                            break;
                        }
                        
                        if(!faces)
                            y += Config::ChunkSize;
                    }

                    if(!faces)
                        break;
                    
                    p_Faces = *faces;
                }
                else
                {
                    // Step 3: Create the bitmask for the face groups.
                    Config::bitmask_t faceCount = CountTrailingOneBits(p_Faces >> heightPos);
                    Config::bitmask_t mask = Config::BitmaskMax;
                    if(faceCount != Config::ChunkSize)
                        mask = (((Config::bitmask_t)1 << faceCount) - 1) << heightPos;

                    // Config::bitmask_t mask = _Faces;
                    // if(mask != Config::BitmaskMax)
                    //     mask = (((Config::bitmask_t)1 << faceCount) - 1) << heightPos;

                    bitmasks.push_back(mask);
                    heightPos += faceCount;
                    totalHeight += faceCount;
                }
            }

            if(bitmasks.size() > 0)
            {
                // Step 4: Find the biggest face
                auto position = p_Context.Position;

                unsigned width = 1;
                position.v[p_Context.Axis.y] = y;

                for (position.v[p_Context.Axis.x] = x + 1; position.v[p_Context.Axis.x] <= p_Context.ModelBBox.End.v[p_Context.Axis.x]; position.v[p_Context.Axis.x]++)
                {
                    bool isContinues = false;
                    fast_vector<std::pair<Config::bitmask_t*, Config::bitmask_t>> nextFaces;

                    // Gets the current chunks mask
                    auto chunkpos = GetChunkpos(position);
                    for (auto &&bitmask : bitmasks)
                    {
                        auto nextfaces = GetFaces(p_Context, chunkpos, d, position.v[p_Context.Axis.x], p_IsFront);
                        if(!nextfaces)
                            break;

                        if((*nextfaces & bitmask) != bitmask)
                        {
                            isContinues = false;
                            break;
                        }

                        // *nextfaces ^= bitmask;
                        nextFaces.push_back({nextfaces, bitmask});
                        isContinues = true;

                        chunkpos.v[p_Context.Axis.y] += Config::ChunkSize;
                    }
                    
                    if(isContinues)
                    {
                        for (auto &&pair : nextFaces)
                            *pair.first ^= pair.second;
                        
                        width++;
                    }
                    else
                        break;
                }

                // Step 5: Build the mesh faces
                position.v[p_Context.Axis.x] = x;
                position.v[p_Context.Axis.y] = y;

                if(!p_IsFront)
                    position.v[p_Context.Axis.z] += 1;

                Math::Vec3f normal;
                normal.v[p_Context.Axis.z] = p_IsFront ? -1 : 1;

                Math::Vec3i size;
                size.v[p_Context.Axis.z] = 0;
                size.v[p_Context.Axis.y] = totalHeight;
                size.v[p_Context.Axis.x] = width;

                Math::Vec3f du;
                du.v[p_Context.Axis.x] = size.v[p_Context.Axis.x];

                Math::Vec3f dv;
                dv.v[p_Context.Axis.y] = size.v[p_Context.Axis.y];

                auto key = p_Context.SliceIt->first;
                auto voxel = CVoxel(key & 0xFFFFFFFF);//*(CVoxel*)&key;
                if(currentMaterial != voxel.GetMaterial())
                {
                    currentMaterial = voxel.GetMaterial();
                    p_Context.Builder.SelectSurface(currentMaterial);
                }

                // TODO: Currently no UVs! How to map Texture?
                // Math::Vec2f uv;
                // if(m_GenerateTexture && !_Context.Model->Textures.empty())
                // {
                //     Math::Vec2ui textureSize(size.v[_Context.Axis.x] + 2, size.v[_Context.Axis.y] + 2);
                //     auto pos = position;
                //     if(!_IsFront)
                //         pos.v[_Context.Axis.z] -= 1;

                //     uv.x = AddTexture(pos, _Context.Axis, textureSize);
                // }
                // else if(_Context.Builder.GetTextures() && !_Context.Builder.GetTextures()->empty())
                //     uv = Math::Vec2f(((float)(voxel.Color + 0.5f)) / _Context.Builder.GetTextures()->at(TextureType::DIFFIUSE)->GetSize().x, 0.5f);

                auto color = voxel.GetColor();
                uint32_t idx1 = p_Context.Builder.AddVertex(new SVertex(position, normal, color));
                uint32_t idx2 = p_Context.Builder.AddVertex(new SVertex(position + du, normal, color));
                uint32_t idx3 = p_Context.Builder.AddVertex(new SVertex(position + dv, normal, color));
                uint32_t idx4 = p_Context.Builder.AddVertex(new SVertex(position + size, normal, color));

                if(p_IsFront)
                    p_Context.Builder.AddFace(idx1, idx2, idx3, idx4);
                else
                    p_Context.Builder.AddFace(idx1, idx3, idx2, idx4);

                y += totalHeight;

                if(static_cast<int>(y & Config::ChunkPositionMask) > chunkpos.v[p_Context.Axis.y])
                {
                    chunkpos.v[p_Context.Axis.y] = y & Config::ChunkPositionMask;
                    Config::bitmask_t *faces = nullptr;

                    // Loops until the next chunk is reached or no more chunks follows the current one.
                    while (!faces)
                    {
                        if(chunkpos.v[p_Context.Axis.y] > p_Context.ModelBBox.End.v[p_Context.Axis.y])
                            break;

                        faces = GetFaces(p_Context, chunkpos, d, x, p_IsFront);
                        if(!faces)
                        {
                            chunkpos.v[p_Context.Axis.y] += Config::ChunkSize;
                            y += Config::ChunkSize;
                        }
                    }

                    // auto faces = GetFaces(_Context, chunkpos, d, x, _IsFront);
                    if(!faces)
                        break;

                    p_Faces = *faces;
                }
            }
            else
                break;
        }
    }

    Mesh CGreedyMesher::GenerateMeshSlices(const VoxelModel &p_Model, const CBBox &p_ModelBBox, int p_RunAxis, int p_AxisPos)
    {
        MeshSlicerContext ctx(p_Model, p_ModelBBox, m_SurfaceFactory);

        // This logic calculates the index of one of the three other axis.
        // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
        // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y
        ctx.Axis = Math::Vec3i((p_RunAxis + 2) % 3, (p_RunAxis + 1) % 3, p_RunAxis);

        CFaceMask mask;
        mask.GroupAfterMaterial = m_GenerateTexture;
        for (int x = p_ModelBBox.Beg.v[ctx.Axis.x]; x <= p_ModelBBox.End.v[ctx.Axis.x]; x++)
        {
            for (uint32_t d = p_AxisPos; d < p_AxisPos + Config::ChunkSize; d++)              
            {
                ctx.Position.v[ctx.Axis.z] = d;
                ctx.Position.v[ctx.Axis.x] = x;
                ctx.Position.v[ctx.Axis.y] = p_ModelBBox.Beg.v[ctx.Axis.y] & Config::ChunkPositionMask;

                while (true)
                {
                    // Gets the current chunks mask
                    auto chunkpos = GetChunkpos(ctx.Position);
                    auto it = ctx.Chunks.find(chunkpos);
                    if(it == ctx.Chunks.end())
                        it = ctx.Chunks.insert({chunkpos, mask.Generate(p_Model, chunkpos, p_RunAxis)}).first;

                    // Checks if there is a slice for the current depth
                    ctx.DepthIt = it->second.find(d & Config::InnerChunkMask);
                    if(ctx.DepthIt != it->second.end())
                    {
                        ctx.SliceIt = ctx.DepthIt->second.begin();
                        while (ctx.SliceIt != ctx.DepthIt->second.end())
                        {
                            Config::bitmask_t faces = ctx.SliceIt->second.Bits[x & Config::InnerChunkMask];
                            GenerateMeshSlice(ctx, faces, true);

                            faces = ctx.SliceIt->second.Bits[(x & Config::InnerChunkMask) + Config::ChunkSize];
                            GenerateMeshSlice(ctx, faces, false);

                            ctx.SliceIt++;
                        }

                        break;
                    }
                    else
                    {
                        if(ctx.Position.v[ctx.Axis.y] >= p_ModelBBox.End.v[ctx.Axis.y])
                            break;

                        ctx.Position.v[ctx.Axis.y] += Config::ChunkSize;
                        // ctx.Position.v[ctx.Axis.y] &= Config::ChunkPositionMask;
                    }
                }
            }
        }

        return ctx.Builder.Build();
    }
}
