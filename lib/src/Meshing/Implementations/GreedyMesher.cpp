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
#include "../../Misc/TexturePacker.hpp"
#include <VCore/Meshing/Mesh/MeshBuilder.hpp>
#include <vector>

#include "GreedyMesher.hpp"

namespace VCore
{
    constexpr static uint32_t g_ChunkSizeP2 = (Config::ChunkSize << 1);
    constexpr static uint32_t g_Mask64 = ~(g_ChunkSizeP2 - 1);

    template<typename R>
    bool is_ready(std::future<R> const& f)
    { return f.wait_for(std::chrono::microseconds(0)) == std::future_status::ready; }

    inline Math::Vec3i GetChunkpos64(Math::Vec3i _Position, const Math::Vec3i &_Axis)
    {
        _Position = GetChunkpos(_Position);
        _Position.v[_Axis.y] &= g_Mask64;

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
            auto end = GetChunkpos(bbox.End).v[runAxis] + Config::ChunkSize;

            for (int axis = begin; axis < end; axis += Config::ChunkSize)
            {
                futures.push_back(std::async(&CGreedyMesher::GenerateMeshSlices, this, _Mesh, bbox, runAxis, axis));
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

        if(m_GenerateTexture)
        {
            TTexturePacker<TextureInfo> packer;
            auto head = m_Head.load();
            while (head)
            {
                packer.AddRect(head->Info.Size, std::move(head->Info));
                head = head->Next;
            }
            
            auto rects = packer.Pack();
            ankerl::unordered_dense::map<uint32_t, decltype(rects)::value_type> textureMapping;
            auto texture = std::make_shared<CTexture>(packer.GetCanvasSize());

            for (auto &&rect : rects)
            {
                textureMapping[rect.Reference.Id] = rect;
                CopyToAtlas(texture, _Mesh, rect.Position, rect.Reference);


                // texture->AddRawPixels(rect.Reference.RawTexture, rect.Position, rect.Size);
            }

            chunk.MeshData->Textures[TextureType::DIFFIUSE] = texture;
            
            for (auto &&surface : chunk.MeshData->Surfaces)
            {
                for (uint32_t i = 0; i < surface->GetVertexCount(); i += 4)
                {
                    auto v1 = surface->GetVertex(i);
                    auto v2 = surface->GetVertex(i + 1);
                    auto v3 = surface->GetVertex(i + 2);
                    auto v4 = surface->GetVertex(i + 3);
                    auto textureRect = textureMapping[v1.UV.x];

                    // For each position we need to substract the margin pixels.
                    v1.UV = Math::Vec2f(textureRect.Position.x + 1, textureRect.Position.y + 1) / packer.GetCanvasSize();
                    v2.UV = Math::Vec2f((textureRect.Position.x + 1) + (textureRect.Size.x - 2), textureRect.Position.y + 1) / packer.GetCanvasSize();
                    v3.UV = Math::Vec2f(textureRect.Position.x + 1, (textureRect.Position.y + 1) + (textureRect.Size.y - 2)) / packer.GetCanvasSize();
                    v4.UV = Math::Vec2f((textureRect.Position + Math::Vec2f(1, 1)) + (textureRect.Size - Math::Vec2f(2, 2))) / packer.GetCanvasSize();

                    surface->UpdateVertex(i, v1);
                    surface->UpdateVertex(i + 1, v2);
                    surface->UpdateVertex(i + 2, v3);
                    surface->UpdateVertex(i + 3, v4);
                }
            }

            ClearTextures();
        }

        return {chunk};
    }

    void CGreedyMesher::CopyToAtlas(Texture &_Atlas, const VoxelModel &_Model, const Math::Vec2ui &_Position, const TextureInfo &_Info)
    {
        auto diffuse = _Model->Textures.find(TextureType::DIFFIUSE);
        if(diffuse != _Model->Textures.end())
        {
            auto size = _Info.Size - Math::Vec2ui(2, 2);

            for (int x = 0; x < size.x; x++)
            {
                for (int y = 0; y < size.y; y++)
                {
                    auto pos = _Info.Position;
                    pos.v[_Info.Axis.x] += x;
                    pos.v[_Info.Axis.y] += y;

                    auto vox = _Model->GetVoxel(pos);
                    if(vox)
                    {
                        auto pixel = diffuse->second->GetPixel(Math::Vec2ui(vox->Color, 0));
                        _Atlas->AddPixel(pixel, _Position + Math::Vec2ui(x + 1, y + 1));

                        // Adds margin pixels to the texture
                        if((x == 0) && (y == 0))
                            _Atlas->AddPixel(pixel, _Position + Math::Vec2ui(x, y));

                        if(x == 0)
                            _Atlas->AddPixel(pixel, _Position + Math::Vec2ui(x, y + 1));

                        if(y == 0)
                            _Atlas->AddPixel(pixel, _Position + Math::Vec2ui(x + 1, y));
                        
                        if(((x + 1) == size.x) && ((y + 1) == size.y))
                            _Atlas->AddPixel(pixel, _Position + Math::Vec2ui(x + 2, y + 2));

                        if((x + 1) == size.x)
                            _Atlas->AddPixel(pixel, _Position + Math::Vec2ui(x + 2, y + 1));

                        if((y + 1) == size.y)
                            _Atlas->AddPixel(pixel, _Position + Math::Vec2ui(x + 1, y + 2));

                        // Top right
                        if(((x + 1) == size.x) && (y == 0))
                            _Atlas->AddPixel(pixel, _Position + Math::Vec2ui(x + 2, y));

                        // Bottom left
                        if((x == 0) && ((y + 1) == size.y))
                            _Atlas->AddPixel(pixel, _Position + Math::Vec2ui(x, y + 2));
                    }
                }
            }
        }
    }

    void CGreedyMesher::GenerateQuad(CMeshBuilder &result, const std::vector<Material> &_Materials, Config::bitmask_t faces, CFaceMask::Mask &bits, int width, int depth, bool isFront, const Math::Vec3i &axis, const SChunkMeta &_Chunk, const Voxel _Voxel)
    {
        int currentMaterial = -1;

        Config::bitmask_t heightPos = 0;
        // Shift werid = hang
        while ((heightPos <= (Config::ChunkSize + 2)) && (faces >> heightPos))
        {
            heightPos += CountTrailingZeroBits(faces >> heightPos);
            if(heightPos >= Config::ChunkSize)
                break;

            Config::bitmask_t faceCount = CountTrailingOneBits(faces >> heightPos);
            Config::bitmask_t mask = (((Config::bitmask_t)1 << faceCount) - 1) << heightPos;

            unsigned w = 1;
            for (int tmpWidth = width + 1; tmpWidth < Config::ChunkSize; tmpWidth++)
            {
                auto &nextfaces = bits.Bits[tmpWidth + ((1 - (int)isFront) * Config::ChunkSize)];
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

    uint32_t CGreedyMesher::AddTexture(const Math::Vec3i &_Position, const Math::Vec3i &_Axis, const Math::Vec2ui &_Size)
    {
        // std::lock_guard<std::mutex> lock(m_Lock);

        uint32_t id = m_NextId++;

        // auto node = new TextureNode(id, _Position, _Axis, _Size);
        TextureNode *node = m_Pool.construct(id, _Position, _Axis, _Size);
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

                    for (int widthAxis = 0; widthAxis < Config::ChunkSize; widthAxis++)
                    {
                        auto faces = key.second.Bits[widthAxis];
                        GenerateQuad(builder, materials, faces, key.second, widthAxis, depth.first, true, Math::Vec3i(axis, axis1, axis2), _Chunk, voxel);

                        faces = key.second.Bits[widthAxis + Config::ChunkSize];
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
            maskGenerator.GroupAfterMaterial = m_GenerateTexture;
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

    Config::bitmask_t *CGreedyMesher::GetFaces(MeshSlicerContext &_Context, const Math::Vec3i &_Chunkpos, int d, int x, bool _IsFront)
    {
        Config::bitmask_t *faces = nullptr;
        auto mask = GetFaceMask(_Context, _Chunkpos, d);
        if(mask)
            faces = &mask->Bits[(x & Config::InnerChunkMask) + ((1 - (int)_IsFront) * Config::ChunkSize)];

        return faces;     
    }

    void CGreedyMesher::GenerateMeshSlice(MeshSlicerContext &_Context, Config::bitmask_t _Faces, bool _IsFront)
    {
        unsigned currentMaterial = -1;
        const int d = _Context.Position.v[_Context.Axis.z] & Config::InnerChunkMask;
        const int x = _Context.Position.v[_Context.Axis.x];
        int y = _Context.Position.v[_Context.Axis.y];
        
        while (y <= _Context.ModelBBox.End.v[_Context.Axis.y])
        {
            fast_vector<Config::bitmask_t> bitmasks;
            Config::bitmask_t heightPos = y & (g_ChunkSizeP2 - 1);
            Config::bitmask_t totalHeight = 0;

            auto position = _Context.Position;
            position.v[_Context.Axis.y] = y;
            auto chunkpos = GetChunkpos64(position, _Context.Axis);

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
                    Config::bitmask_t faceCount = CountTrailingOneBits(_Faces >> heightPos);
                    Config::bitmask_t mask = _Faces;
                    if(mask != Config::BitmaskMax)
                        mask = (((Config::bitmask_t)1 << faceCount) - 1) << heightPos;

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

                // Step 5: Build the mesh faces
                position.v[_Context.Axis.x] = x;
                position.v[_Context.Axis.y] = y;

                if(!_IsFront)
                    position.v[_Context.Axis.z] += 1;

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
                if(m_GenerateTexture && !_Context.Model->Textures.empty())
                {
                    Math::Vec2ui textureSize(size.v[_Context.Axis.x] + 2, size.v[_Context.Axis.y] + 2);
                    auto pos = position;
                    if(!_IsFront)
                        pos.v[_Context.Axis.z] -= 1;

                    uv.x = AddTexture(pos, _Context.Axis, textureSize);
                }
                else if(_Context.Builder.GetTextures() && !_Context.Builder.GetTextures()->empty())
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
        MeshSlicerContext ctx(_Model, _ModelBBox, m_SurfaceFactory);

        // This logic calculates the index of one of the three other axis.
        // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
        // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y
        ctx.Axis = Math::Vec3i((_RunAxis + 2) % 3, (_RunAxis + 1) % 3, _RunAxis);

        CFaceMask mask;
        mask.GroupAfterMaterial = m_GenerateTexture;
        for (int x = _ModelBBox.Beg.v[ctx.Axis.x]; x <= _ModelBBox.End.v[ctx.Axis.x]; x++)
        {
            for (int d = _AxisPos; d < _AxisPos + Config::ChunkSize; d++)              
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
                        ctx.Position.v[ctx.Axis.y] += g_ChunkSizeP2;
                        if(ctx.Position.v[ctx.Axis.y] >= _ModelBBox.End.v[ctx.Axis.y])
                            break;
                    }
                }
            }
        }

        return ctx.Builder.Build();
    }
}
