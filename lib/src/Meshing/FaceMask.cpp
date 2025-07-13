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
#include <cmath>
#include <VCore/Meshing/MaterialManager.hpp>

namespace VCore
{
    ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<uint32_t, CFaceMask::Mask>> CFaceMask::Generate(const VoxelModel &_Model, const SChunkMeta &_Chunk, const uint8_t _Axis)
    {
        m_Model = _Model;
        m_Chunk = _Chunk;

        // This logic calculates the index of one of the three other axis.
        m_Axis = Math::TVector3<char>(_Axis, (_Axis + 1) % 3, (_Axis + 2) % 3);

        InternalGenerate();
        return std::move(m_FacesMasks);
    }

    ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<uint32_t, CFaceMask::Mask>> CFaceMask::Generate(const VoxelModel &_Model, Math::Vec3i _ChunkPos, const uint8_t _Axis)
    {
        m_Model = _Model;

        // This logic calculates the index of one of the three other axis.
        m_Axis = Math::TVector3<char>(_Axis, (_Axis + 1) % 3, (_Axis + 2) % 3);

        auto &voxels = *_Model;
        m_Chunk.Chunk = voxels.GetChunk(_ChunkPos);
        if(m_Chunk.Chunk)
        {
            // meta.UniqueId = hasher(_ChunkPos);
            m_Chunk.TotalBBox = CBBox(_ChunkPos, _ChunkPos + Math::Vec3i(Config::ChunkSize, Config::ChunkSize, Config::ChunkSize));
            m_Chunk.InnerBBox = m_Chunk.Chunk->inner_bbox(_ChunkPos);
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

    void CFaceMask::FillVoxelBits(Config::bitmask_t *_opaqueVoxels, Config::bitmask_t *_transparentVoxels, const CChunk *_Chunk, const Math::Vec3i &_Position, const int _Count)
    {
        Math::Vec3i subpos = _Position & Config::InnerChunkMask;
        for (int i = 0; i < _Count; i++)
        {          
            _opaqueVoxels[i] = _Chunk->Mask.GetRowFaces(subpos, m_Axis.y); // (_Chunk->m_Mask.GetRowFaces(subpos, m_Axis.y) >> 1) & 0xFFFFFFFF;

            if(_opaqueVoxels[i] && (m_TransparentMaterials.size() > 0))
            {
                uint32_t bitCount = CountTrailingZeroBits(_opaqueVoxels[i]);
                auto subposCopy = subpos;
                while (bitCount < Config::ChunkSize)
                {
                    auto count = CountTrailingOneBits(_opaqueVoxels[i] >> bitCount);
                    for (uint32_t j = 0; j < count; j++)
                    {
                        subposCopy.v[m_Axis.y] = bitCount + j;
                        auto voxel = _Chunk->find(subposCopy);
                        if(std::find(m_TransparentMaterials.begin(), m_TransparentMaterials.end(), voxel.GetMaterial()) != m_TransparentMaterials.end())
                        {
                            _opaqueVoxels[i] &= ~(1 << (bitCount + j));
                            _transparentVoxels[i] |= (1 << (bitCount + j));
                        }
                    }

                    bitCount += count;
                    bitCount += CountTrailingZeroBits(_opaqueVoxels[i] >> bitCount);
                }
            }

            subpos.v[m_Axis.z]++;
        }
    }

    const int simdIntSize = sizeof(Simd::NativeI) / sizeof(int);

    void CFaceMask::InternalGenerate()
    {
        const CBBox &BBox = m_Chunk.InnerBBox;
        const CBBox &TotalBBox = m_Chunk.TotalBBox;

        m_MaskCache = nullptr;
        m_CachedKey = 0xFFFFFFFF;

        // Finds all transparent materials.
        for (uint8_t i = 0; i < Config::MaxMaterialSlots; i++)
        {
            auto material = MaterialManager::GetMaterial(i);
            if(material && std::fpclassify(material->Transparency) != FP_ZERO)
                m_TransparentMaterials.push_back(i);
        }

        // for (int heightAxis = BBox.Beg.v[axis1]; heightAxis <= BBox.End.v[axis1]; heightAxis++)
        for (int depthAxis = BBox.Beg.v[m_Axis.x]; depthAxis <= BBox.End.v[m_Axis.x]; depthAxis++)
        {
            for (int widthAxis = BBox.Beg.v[m_Axis.z]; widthAxis <= BBox.End.v[m_Axis.z]; widthAxis += simdIntSize)
            {
                // Calculates the remaining amount of elements
                // in the voxel array.
                const int count = ((BBox.End.v[m_Axis.z] - widthAxis) >= simdIntSize) ? simdIntSize : ((BBox.End.v[m_Axis.z] - widthAxis) + 1);

                // Global position of the current column.
                Math::Vec3i position;
                position.v[m_Axis.x] = depthAxis;
                position.v[m_Axis.y] = BBox.Beg.v[m_Axis.y];
                position.v[m_Axis.z] = widthAxis;

                // Position of the voxel in the current chunk.
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
                    if((position.v[m_Axis.x] < TotalBBox.Beg.v[m_Axis.x]) || (position.v[m_Axis.x] >= TotalBBox.End.v[m_Axis.x]))
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

    void CFaceMask::GenerateMask(Config::bitmask_t *_Voxels, const Math::Vec3i &_Subpos, const int _Count)
    {
        Simd::NativeI beforeVoxelsSimd((int*)_Voxels, simdIntSize);
        Simd::NativeI voxelsSimd((int*)_Voxels + simdIntSize, simdIntSize);
        Simd::NativeI afterVoxelsSimd((int*)_Voxels + (simdIntSize * 2), simdIntSize);

        // Cache reset
        ankerl::unordered_dense::map<uint32_t, Mask> *masks = nullptr;
        m_CachedKey = 0xFFFFFFFF;
        m_MaskCache = nullptr;

        Config::bitmask_t frontFaces[simdIntSize] = {};
        Config::bitmask_t backFaces[simdIntSize] = {};

        // With simd and bit manipulation, we find all faces which are not
        // corvered. Since simd works on parallel data, we can check 32 * simdSize faces
        // at the same time.
        ((beforeVoxelsSimd & voxelsSimd) ^ voxelsSimd).Store((int*)frontFaces, simdIntSize);
        ((afterVoxelsSimd & voxelsSimd) ^ voxelsSimd).Store((int*)backFaces, simdIntSize);

        // Fill the mask structure with data.
        for (int i = 0; i < _Count; i++)
        {
            auto faces = (unsigned int)frontFaces[i];
            if(faces)
            {
                if(!masks)
                    masks = &m_FacesMasks[_Subpos.v[m_Axis.x]];
                FillSlice(faces, _Subpos, i, false, *masks);
            }

            faces = (unsigned int)backFaces[i];
            if(faces)
            {
                if(!masks)
                    masks = &m_FacesMasks[_Subpos.v[m_Axis.x]];
                FillSlice(faces, _Subpos, i, true, *masks);
            }
        }
    }

    void CFaceMask::FillSlice(Config::bitmask_t _Faces, const Math::Vec3i &_Subpos, const int _Column, const bool _Backface, ankerl::unordered_dense::map<uint32_t, Mask> &_Masks)
    {
        auto bitCount = CountTrailingZeroBits(_Faces);
        auto subposCopy = _Subpos;
        subposCopy.v[m_Axis.z] += _Column;

        while (bitCount < Config::ChunkSize)
        {
            auto count = CountTrailingOneBits(_Faces >> bitCount);
            for (uint32_t j = 0; j < count; j++)
            {
                subposCopy.v[m_Axis.y] = bitCount + j;
                auto voxel = m_Chunk.Chunk->find(subposCopy);
                if(!voxel.IsInstantiated())
                    continue;

                auto key = (uint32_t)voxel;
                if(GroupAfterMaterial)
                    key = voxel.GetMaterial();

                // Checks if there is already a cached version.
                if((key != m_CachedKey) || !m_MaskCache)
                {
                    m_CachedKey = key;
                    m_MaskCache = &_Masks[m_CachedKey];
                }

                m_MaskCache->Bits[_Subpos.v[m_Axis.z] + _Column + (_Backface * Config::ChunkSize)] |= (Config::bitmask_t)1 << (bitCount + j);
            }

            bitCount += count;
            bitCount += CountTrailingZeroBits(_Faces >> bitCount);
        }
    }
} // namespace VCore
