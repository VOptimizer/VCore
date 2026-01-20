/*
 * MIT License
 *
 * Copyright (c) 2024 Christian Tost
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

#include <algorithm>
#include "FaceMask.hpp"
#include "../Simd/Simd.hpp"
#include "../Misc/Helper.hpp"
#include "VCore/Math/Vector.hpp"
#include <cmath>
#include <VCore/Meshing/MaterialManager.hpp>
#include <cstdint>

namespace VCore
{
    ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<uint64_t, CFaceMask::Mask>> CFaceMask::Generate(const VoxelModel &p_Model, const SChunkMeta &p_Chunk, const uint8_t p_Axis)
    {
        m_Model = p_Model;
        m_Chunk = p_Chunk;

        // This logic calculates the index of each of the three other axis.
        m_Axis = Math::TVector3<char>(p_Axis, (p_Axis + 1) % 3, (p_Axis + 2) % 3);

        InternalGenerate();
        return std::move(m_FacesMasks);
    }

    ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<uint64_t, CFaceMask::Mask>> CFaceMask::Generate(const VoxelModel &p_Model, Math::Vec3i p_ChunkPos, const uint8_t p_Axis)
    {
        m_Model = p_Model;

        // This logic calculates the index of one of the three other axis.
        m_Axis = Math::TVector3<char>(p_Axis, (p_Axis + 1) % 3, (p_Axis + 2) % 3);

        auto &voxels = *p_Model;
        m_Chunk.Chunk = voxels.GetChunk(p_ChunkPos);
        if(m_Chunk.Chunk)
        {
            // meta.UniqueId = hasher(_ChunkPos);
            m_Chunk.TotalBBox = CBBox(p_ChunkPos, p_ChunkPos + Math::Vec3i(Config::ChunkSize, Config::ChunkSize, Config::ChunkSize));
            m_Chunk.InnerBBox = m_Chunk.Chunk->inner_bbox(p_ChunkPos);
            InternalGenerate();
        }

        // _ChunkPos.v[(_Axis + 1) % 3] += Config::ChunkSize;
        // meta.Chunk = voxels.getChunk(_ChunkPos);
        // if(meta.Chunk)
        // {
        //     // meta.UniqueId = hasher(_ChunkPos);
        //     meta.TotalBBox = CBBox(_ChunkPos, _ChunkPos + Math::Vec3i(Config::ChunkSize, Config::ChunkSize, Config::ChunkSize));
        //     meta.InnerBBox = meta.Chunk->inner_bbox(_ChunkPos);
        //     InternalGenerate(_Model, meta, _Axis, ((Config::ChunkSize << 1) - 1));
        // }

        return std::move(m_FacesMasks);
    }

    void CFaceMask::FillVoxelBits(Config::bitmask_t *p_opaqueVoxels, Config::bitmask_t *p_transparentVoxels, const CChunk *p_Chunk, const Math::Vec3i &p_Position, const int p_Count)
    {
        Math::Vec3i subpos = p_Position & Config::InnerChunkMask;
        for (int i = 0; i < p_Count; i++)
        {          
            p_opaqueVoxels[i] = p_Chunk->Mask.GetRowFaces(subpos, m_Axis.y); // (_Chunk->m_Mask.GetRowFaces(subpos, m_Axis.y) >> 1) & 0xFFFFFFFF;

            // Filters for transparent voxels.
            if(p_opaqueVoxels[i] && (m_TransparentMaterials.size() > 0))
            {
                uint32_t bitCount = CountTrailingZeroBits(p_opaqueVoxels[i]);
                auto subposCopy = subpos;
                while (bitCount < Config::ChunkSize)
                {
                    auto count = CountTrailingOneBits(p_opaqueVoxels[i] >> bitCount);
                    for (uint32_t j = 0; j < count; j++)
                    {
                        subposCopy.v[m_Axis.y] = bitCount + j;
                        auto voxel = p_Chunk->find(subposCopy);

                        // Has this voxel a transparent material?
                        if(std::find(m_TransparentMaterials.begin(), m_TransparentMaterials.end(), voxel.GetMaterial()) != m_TransparentMaterials.end())
                        {
                            // Removes the transparent voxel from the opaque ones.
                            p_opaqueVoxels[i] &= ~(1 << (bitCount + j));

                            // Adds the voxel to the transparent ones.
                            p_transparentVoxels[i] |= (1 << (bitCount + j));
                        }
                    }

                    bitCount += count;
                    bitCount += CountTrailingZeroBits(p_opaqueVoxels[i] >> bitCount);
                }
            }

            subpos.v[m_Axis.z]++;
        }
    }

    const int simdIntSize = sizeof(Simd::NativeI) / sizeof(int);

    void CFaceMask::InternalGenerate()
    {
        const CBBox &bbox = m_Chunk.InnerBBox;
        const CBBox &totalBBox = m_Chunk.TotalBBox;

        m_MaskCache = nullptr;
        m_CachedKey = 0x00000003FFFFFFFF;

        // To differentiate between opaque and none opaque voxels, it's neccessary
        // to filter for all materials, which had some kind of transparency enabled.
        for (uint8_t i = 0; i < Config::MaxMaterialSlots; i++)
        {
            auto material = MaterialManager::GetMaterial(i);
            if(material && std::fpclassify(material->Transparency) != FP_ZERO)
                m_TransparentMaterials.push_back(i);
        }

        for (int depthAxis = bbox.Beg.v[m_Axis.x]; depthAxis <= bbox.End.v[m_Axis.x]; depthAxis++)
        {
            for (int widthAxis = bbox.Beg.v[m_Axis.z]; widthAxis <= bbox.End.v[m_Axis.z]; widthAxis += simdIntSize)
            {
                // Calculates the remaining amount of elements
                // in the voxel array.
                const int count = ((bbox.End.v[m_Axis.z] - widthAxis) >= simdIntSize) ? simdIntSize : ((bbox.End.v[m_Axis.z] - widthAxis) + 1);

                // Global position of the current voxel column.
                Math::Vec3i position;
                position.v[m_Axis.x] = depthAxis;
                position.v[m_Axis.y] = bbox.Beg.v[m_Axis.y];
                position.v[m_Axis.z] = widthAxis;

                // Position of the voxel column in the current chunk.
                Math::Vec3i subpos = position & Config::InnerChunkMask;

                // Contains the voxels column before and after as well as the current one.
                // Layout:
                // | Bytes                    | Meaning |
                // |:------------------------ |:------- |
                // | 0 -  simdSize            | Voxel column before the current one |
                // | simdSize - simdSize*2    | Current voxel column    |
                // | simdSize*2 - simdSize*3  | Voxel column after the current one  |
                Config::bitmask_t opaqueVoxels[simdIntSize * 3] = {};
                Config::bitmask_t transparentVoxels[simdIntSize * 3] = {};

                // Go before the current voxel column.
                position.v[m_Axis.x]--;

                // Fill the voxel array.
                for (int i = 0; i < 3; i++)
                {
                    auto chunk = m_Chunk.Chunk;

                    // Checks if the position is outside of the current chunk and gets the neighbor chunk
                    if((position.v[m_Axis.x] < totalBBox.Beg.v[m_Axis.x]) || (position.v[m_Axis.x] >= totalBBox.End.v[m_Axis.x]))
                        chunk = m_Model->GetChunk(position);

                    if(chunk)
                        FillVoxelBits(opaqueVoxels + (i * simdIntSize), transparentVoxels + (i * simdIntSize), chunk, position, count);
                    position.v[m_Axis.x]++;
                }

                GenerateMask(opaqueVoxels, subpos, count);
                if(m_TransparentMaterials.size() > 0)
                    GenerateMask(transparentVoxels, subpos, count);
            }
        }

        m_TransparentMaterials.clear();
    }

    void CFaceMask::GenerateMask(Config::bitmask_t *p_Voxels, const Math::Vec3i &p_Subpos, const int p_Count)
    {
        Simd::NativeI beforeVoxelsSimd((int*)p_Voxels, simdIntSize);
        Simd::NativeI voxelsSimd((int*)p_Voxels + simdIntSize, simdIntSize);
        Simd::NativeI afterVoxelsSimd((int*)p_Voxels + (simdIntSize * 2), simdIntSize);

        // Cache reset
        ankerl::unordered_dense::map<uint64_t, Mask> *masks = nullptr;
        m_CachedKey = 0xFFFFFFFF;
        m_MaskCache = nullptr;

        // Visible faces, which are not covered by other voxels.
        Config::bitmask_t frontFaces[simdIntSize] = {};
        Config::bitmask_t backFaces[simdIntSize] = {};

        // With simd and bit manipulation, we find all faces which are not
        // covered. Since simd works on parallel data, we can check 32 * simdSize faces
        // at the same time.
        ((beforeVoxelsSimd & voxelsSimd) ^ voxelsSimd).Store((int*)frontFaces, simdIntSize);
        ((afterVoxelsSimd & voxelsSimd) ^ voxelsSimd).Store((int*)backFaces, simdIntSize);

        // Fill the mask structure with data.
        for (int i = 0; i < p_Count; i++)
        {
            auto faces = (unsigned int)frontFaces[i];
            if(faces)
            {
                if(!masks)
                    masks = &m_FacesMasks[p_Subpos.v[m_Axis.x]];
                FillSlice(faces, p_Subpos, i, false, *masks);
            }

            faces = (unsigned int)backFaces[i];
            if(faces)
            {
                if(!masks)
                    masks = &m_FacesMasks[p_Subpos.v[m_Axis.x]];
                FillSlice(faces, p_Subpos, i, true, *masks);
            }
        }
    }

    inline uint8_t GenerateAO(uint8_t p_Side1, uint8_t p_Side2, uint8_t p_Corner)
    {
        if(p_Side1 && p_Side2)
            return 0;

        return 3 - (p_Side1 + p_Side2 + p_Corner);
    }

    uint8_t CFaceMask::CalculateAo(const Math::Vec3i &p_GlobalPos, const Math::Vec2i *p_Lookup)
    {
        uint8_t sides[3] = {};

        for (int i = 0; i < 3; i++) 
        {
            auto copyGlobal = p_GlobalPos;
            copyGlobal.v[m_Axis.y] += p_Lookup[i].y;
            copyGlobal.v[m_Axis.z] += p_Lookup[i].x;

            if(m_Chunk.TotalBBox.ContainsPoint(copyGlobal))
                sides[i] = m_Chunk.Chunk->HasVoxel(copyGlobal);
            else
                sides[i] = m_Model->HasVoxel(copyGlobal);
        }

        return GenerateAO(sides[0], sides[1], sides[2]);
    }

    static const Math::Vec2i LEFT_BOTTOM_LOOKUP[] = {
        Math::Vec2i(-1, 0), Math::Vec2i(0, -1), Math::Vec2i(-1, -1)
    };

    static const Math::Vec2i RIGHT_BOTTOM_LOOKUP[] = {
        Math::Vec2i(1, 0), Math::Vec2i(0, -1), Math::Vec2i(1, -1)
    };

    static const Math::Vec2i LEFT_TOP_LOOKUP[] = {
        Math::Vec2i(-1, 0), Math::Vec2i(0, 1), Math::Vec2i(-1, 1)
    };

    static const Math::Vec2i RIGHT_TOP_LOOKUP[] = {
        Math::Vec2i(1, 0), Math::Vec2i(0, 1), Math::Vec2i(1, 1)
    };

    uint8_t CFaceMask::CalculateAo(const Math::Vec3i &p_Subpos, const bool p_Backface)
    {
        auto globalPos = m_Chunk.TotalBBox.Beg + p_Subpos;
        
        // Gets the voxel above the current surface, in normal direction.
        globalPos.v[m_Axis.x] += p_Backface ? 1 : -1;

        uint16_t ao = 0;

        // Left-Bottom
        ao = CalculateAo(globalPos, LEFT_BOTTOM_LOOKUP);

        // Right-Bottom
        ao |= CalculateAo(globalPos, RIGHT_BOTTOM_LOOKUP) << 2;

        // Left-Top
        ao |= CalculateAo(globalPos, LEFT_TOP_LOOKUP) << 4;

        // Right-Top
        ao |= CalculateAo(globalPos, RIGHT_TOP_LOOKUP) << 6;

        return ao;
    }

    void CFaceMask::FillSlice(Config::bitmask_t p_Faces, const Math::Vec3i &p_Subpos, const int p_Column, const bool p_Backface, ankerl::unordered_dense::map<uint64_t, Mask> &p_Masks)
    {
        auto bitCount = CountTrailingZeroBits(p_Faces);
        auto subposCopy = p_Subpos;
        subposCopy.v[m_Axis.z] += p_Column;

        while (bitCount < Config::ChunkSize)
        {
            auto count = CountTrailingOneBits(p_Faces >> bitCount);
            for (uint32_t j = 0; j < count; j++)
            {
                subposCopy.v[m_Axis.y] = bitCount + j;
                auto voxel = m_Chunk.Chunk->find(subposCopy);
                if(!voxel.IsInstantiated())
                    continue;

                uint64_t key = (uint32_t)voxel;
                if(GroupAfterMaterial)
                    key = voxel.GetMaterial();

                key |= ((uint64_t)CalculateAo(subposCopy, p_Backface)) << 32;

                // Checks if there is already a cached version.
                if((key != m_CachedKey) || !m_MaskCache)
                {
                    m_CachedKey = key;
                    m_MaskCache = &p_Masks[m_CachedKey];
                }

                m_MaskCache->Bits[p_Subpos.v[m_Axis.z] + p_Column + (p_Backface * Config::ChunkSize)] |= (Config::bitmask_t)1 << (bitCount + j);
            }

            bitCount += count;
            bitCount += CountTrailingZeroBits(p_Faces >> bitCount);
        }
    }
} // namespace VCore
