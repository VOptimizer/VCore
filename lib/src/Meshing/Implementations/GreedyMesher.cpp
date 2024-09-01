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
       std::vector<SMeshChunk> ret;

        CVoxelSpace::querylist chunks;
        if(m_Frustum)
            chunks = _Mesh->QueryChunks(m_Frustum);
        else
        {
            if(!_OnlyDirty)
                chunks = _Mesh->QueryChunks();
            else
                chunks = _Mesh->QueryDirtyChunks();
        }

        CSliceCollection collection;

        std::vector<std::future<std::pair<const SChunkMeta&, CSliceCollection>>> futures;
        for (auto &&c : chunks)
        {
            _Mesh->GetVoxels().markAsProcessed(c);
            futures.push_back(std::async(&CGreedyMesher::GenerateSlicedChunk, this, _Mesh, c, true));
            while(futures.size() >= std::thread::hardware_concurrency())
            {
                auto it = futures.begin();
                while (it != futures.end())
                {
                    if(is_ready(*it))
                    {
                        auto result = it->get();
                        if(m_GenerateSingleChunks)
                            ret.push_back(GenerateMeshChunk(_Mesh, result.second, &result.first));
                        else
                            collection.Merge(result.second);                     
                        it = futures.erase(it);
                    }
                    else
                        it++;
                }
            }
        }

        auto it = futures.begin();
        while (it != futures.end())
        {
            it->wait();
            auto result = it->get();
            if(m_GenerateSingleChunks)
                ret.push_back(GenerateMeshChunk(_Mesh, result.second, &result.first));
            else
                collection.Merge(result.second);         
            it = futures.erase(it);
        }

        if(!m_GenerateSingleChunks)
            ret.push_back(GenerateMeshChunk(_Mesh, collection));
        
        return ret;
    }

    SMeshChunk CGreedyMesher::GenerateMeshChunk(VoxelModel _Mesh, CSliceCollection &_Collection, const SChunkMeta *_Chunk)
    {
        CMeshBuilder builder(m_SurfaceFactory);
        auto textures = _Mesh->Textures;
        auto &materials = _Mesh->Materials;

        _Collection.Optimize(m_GenerateTexture);
        if(m_GenerateTexture)
            textures = _Collection.Textures;

        builder.AddTextures(textures);

        // Generate the mesh.
        for (size_t runAxis = 0; runAxis < 3; runAxis++)
        {
            auto &slices = _Collection.mSlices[runAxis];
            for (auto &&slice : slices) // std::map<int, Slice>;
            {
                for (auto &&height : slice.second) // std::map<int, Quads>;
                {
                    for (auto &&quad : height.second)
                    {
                        int heightAxis = (runAxis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
                        int widthAxis = (runAxis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y

                        Math::Vec3f du;
                        du.v[widthAxis] = quad.mQuad.second.v[widthAxis];

                        Math::Vec3f dv;
                        dv.v[heightAxis] = quad.mQuad.second.v[heightAxis];

                        Math::Vec3f v1 = quad.mQuad.first;
                        Math::Vec3f v2 = quad.mQuad.first + du;
                        Math::Vec3f v3 = quad.mQuad.first + dv;
                        Math::Vec3f v4 = quad.mQuad.first + quad.mQuad.second;

                        Material mat;
                        if(quad.Material < (int)materials.size())
                            mat = materials[quad.Material];

                        if(m_GenerateTexture)
                        {
                            Math::Vec3f faceNormal = (v2 - v1).cross(v3 - v1).normalize();

                            if(faceNormal == quad.Normal)
                            {
                                builder.AddFace(
                                    SVertex(v1, quad.Normal, Math::Vec2f(quad.UvStart) / Math::Vec2f(_Collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    SVertex(v2, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(du.v[widthAxis], 0)) / Math::Vec2f(_Collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    SVertex(v3, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(0, -dv.v[heightAxis])) / Math::Vec2f(_Collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    mat);

                                builder.AddFace(
                                    SVertex(v2, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(du.v[widthAxis], 0)) / Math::Vec2f(_Collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    SVertex(v4, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(du.v[widthAxis], -dv.v[heightAxis])) / Math::Vec2f(_Collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    SVertex(v3, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(0, -dv.v[heightAxis])) / Math::Vec2f(_Collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    mat);
                            } 
                            else
                            {
                                builder.AddFace(
                                    SVertex(v3, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(0, -dv.v[heightAxis])) / Math::Vec2f(_Collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    SVertex(v2, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(du.v[widthAxis], 0)) / Math::Vec2f(_Collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    SVertex(v1, quad.Normal, Math::Vec2f(quad.UvStart) / Math::Vec2f(_Collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    mat);

                                builder.AddFace(
                                    SVertex(v3, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(0, -dv.v[heightAxis])) / Math::Vec2f(_Collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    SVertex(v4, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(du.v[widthAxis], -dv.v[heightAxis])) / Math::Vec2f(_Collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    SVertex(v2, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(du.v[widthAxis], 0)) / Math::Vec2f(_Collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    mat);
                            }
                        }
                        else
                            builder.AddFace(v1, v2, v3, v4, quad.Normal, quad.Color, mat);
                    }
                }
            }
        }

        SMeshChunk chunk;
        if(_Chunk)
        {
            chunk.UniqueId = _Chunk->UniqueId;
            chunk.InnerBBox = _Chunk->InnerBBox;
            chunk.TotalBBox = _Chunk->TotalBBox;
        }
        else
        {
            Math::Vec3iHasher hasher;
            auto bbox = _Mesh->GetBBox();

            chunk.UniqueId = hasher(bbox.Beg);
            chunk.InnerBBox = bbox;
            chunk.TotalBBox = bbox;
        }

        chunk.MeshData = builder.Build();
        return chunk;
    }

    std::pair<const SChunkMeta&, CSliceCollection> CGreedyMesher::GenerateSlicedChunk(VoxelModel m, const SChunkMeta &_Chunk, bool)
    {
        CBBox BBox = _Chunk.InnerBBox;
        CSliceCollection result;

        auto albedo = m->Textures[TextureType::DIFFIUSE];
        Texture emission;
        auto textureIt = m->Textures.find(TextureType::EMISSION);
        if(textureIt != m->Textures.end())
            emission = textureIt->second;

        // For all 3 axis (x, y, z)
        for (size_t axis = 0; axis < 3; axis++)
        {
            // This logic calculates the index of one of the three other axis.
            int axis1 = (axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
            int axis2 = (axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y

            CFaceMask mask;
            auto masks = mask.Generate(m, _Chunk, axis);

            for (auto &&depth : masks)
            {
                result.AddSlice(axis, depth.first);
                for (auto &&key : depth.second)
                {
                    auto parts = split(key.first, "_");

                    for (int widthAxis = 0; widthAxis < CHUNK_SIZE; widthAxis++)
                    {
                        auto faces = key.second.Bits[widthAxis];
                        GenerateQuad(result, faces, key.second, widthAxis, depth.first, true, Math::Vec3i(axis, axis1, axis2), _Chunk, parts);

                        faces = key.second.Bits[widthAxis + CHUNK_SIZE];
                        GenerateQuad(result, faces, key.second, widthAxis, depth.first + 1, false, Math::Vec3i(axis, axis1, axis2), _Chunk, parts);
                    }
                }
            }
        }

        return std::pair<const SChunkMeta&, CSliceCollection>(std::move(_Chunk), std::move(result));
    }

    void CGreedyMesher::GenerateQuad(CSliceCollection &result, BITMASK_TYPE faces, CFaceMask::Mask &bits, int width, int depth, bool isFront, const Math::Vec3i &axis, const SChunkMeta &_Chunk, const std::vector<std::string> &parts)
    {
        unsigned heightPos = 0;
        // Shift werid = hang
        while ((heightPos <= (CHUNK_SIZE + 2)) && (faces >> heightPos))
        {
            heightPos += CountTrailingZeroBits(faces >> heightPos);
            auto faceCount = CountTrailingOneBits(faces >> heightPos);
            BITMASK_TYPE mask = ((1 << faceCount) - 1) << heightPos;

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

            result.AddQuadInfo(axis.x, depth, position.v[axis.y], CQuadInfo({position, size}, normal, std::stoi(parts[0]), std::stoi(parts[1])));
            heightPos += faceCount;
        }
    }
}
