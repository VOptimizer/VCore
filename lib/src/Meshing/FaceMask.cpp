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

#include "FaceMask.hpp"
#include "../Misc/Helper.hpp"
#include <cmath>

namespace VCore
{
    ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<uint32_t, CFaceMask::Mask>> CFaceMask::Generate(const VoxelModel &_Model, const SChunkMeta &_Chunk, uint8_t _Axis)
    {
        InternalGenerate(_Model, _Chunk, _Axis, Config::InnerChunkMask);
        return std::move(m_FacesMasks);
    }

    ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<uint32_t, CFaceMask::Mask>> CFaceMask::Generate(const VoxelModel &_Model, Math::Vec3i _ChunkPos, uint8_t _Axis)
    {
        SChunkMeta meta;
        auto &voxels = _Model->GetVoxels();
        meta.Chunk = voxels.getChunk(_ChunkPos);
        if(meta.Chunk)
        {
            // meta.UniqueId = hasher(_ChunkPos);
            meta.TotalBBox = CBBox(_ChunkPos, _ChunkPos + Math::Vec3i(Config::ChunkSize, Config::ChunkSize, Config::ChunkSize));
            meta.InnerBBox = meta.Chunk->inner_bbox(_ChunkPos);
            InternalGenerate(_Model, meta, _Axis, ((Config::ChunkSize << 1) - 1));
        }

        _ChunkPos.v[(_Axis + 1) % 3] += Config::ChunkSize;
        meta.Chunk = voxels.getChunk(_ChunkPos);
        if(meta.Chunk)
        {
            // meta.UniqueId = hasher(_ChunkPos);
            meta.TotalBBox = CBBox(_ChunkPos, _ChunkPos + Math::Vec3i(Config::ChunkSize, Config::ChunkSize, Config::ChunkSize));
            meta.InnerBBox = meta.Chunk->inner_bbox(_ChunkPos);
            InternalGenerate(_Model, meta, _Axis, ((Config::ChunkSize << 1) - 1));
        }

        return std::move(m_FacesMasks);
    }

    void CFaceMask::InternalGenerate(const VoxelModel &_Model, const SChunkMeta &_Chunk, uint8_t _Axis, int _ChunkMask)
    {
        const CBBox &BBox = _Chunk.InnerBBox;

        // This logic calculates the index of one of the three other axis.
        int axis1 = (_Axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
        int axis2 = (_Axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y

        for (int heightAxis = BBox.Beg.v[axis1]; heightAxis <= BBox.End.v[axis1]; heightAxis++)
        {
            for (int widthAxis = BBox.Beg.v[axis2]; widthAxis <= BBox.End.v[axis2]; widthAxis++)
            {
                Math::Vec3i position;
                position.v[_Axis] = 0;
                position.v[axis1] = heightAxis;
                position.v[axis2] = widthAxis;

                Math::Vec3i subpos = position & Config::InnerChunkMask;

                // Gets the current "ray" of bits.
                auto voxels = _Chunk.Chunk->m_Mask.GetRowFaces(subpos, _Axis);
                if(!voxels)
                    continue;

                // Splits the bits into opaque and transparent ones.
                auto mask = GenerateOpaqueMask(_Model, _Chunk, voxels, position, _Axis);

                // Generates a mask of all in a "ray" visible faces!
                Config::bitmask_t frontFaces = (mask.Opaque & (Config::bitmask_t)~(mask.Opaque << 1)) >> 1;
                Config::bitmask_t backFaces = ((mask.Opaque & (Config::bitmask_t)~(mask.Opaque >> 1)) >> 1) & Config::FaceMask;

                if(mask.Opaque)
                {
                    GenerateMask(frontFaces, false, position, Math::Vec3i(_Axis, axis1, axis2), _Chunk, _ChunkMask);
                    GenerateMask(backFaces, true, position, Math::Vec3i(_Axis, axis1, axis2), _Chunk, _ChunkMask);
                }

                if(mask.Transparent)
                {
                    Config::bitmask_t transparentFrontFaces = (mask.Transparent & (Config::bitmask_t)~(mask.Transparent << 1)) >> 1;
                    Config::bitmask_t transparentBackFaces = ((mask.Transparent & (Config::bitmask_t)~(mask.Transparent >> 1)) >> 1) & Config::FaceMask;

                    GenerateMask(transparentFrontFaces & ~(frontFaces << 1), false, position, Math::Vec3i(_Axis, axis1, axis2), _Chunk, _ChunkMask);
                    GenerateMask(transparentBackFaces & ~(backFaces >> 1), true, position, Math::Vec3i(_Axis, axis1, axis2), _Chunk, _ChunkMask);
                }
            }
        }
    }

    CFaceMask::OpaqueMask CFaceMask::GenerateOpaqueMask(const VoxelModel &_Model, const SChunkMeta &_Chunk, Config::bitmask_t _Voxels, Math::Vec3i position, uint8_t _Axis)
    {
        OpaqueMask mask;

        auto totalBeg = _Chunk.TotalBBox.Beg.v[_Axis];
        auto totalEnd = _Chunk.TotalBBox.End.v[_Axis];
        auto &posAxis = position.v[_Axis];

        Config::bitmask_t pos = 0;
        while ((pos <= (Config::ChunkSize + 2)) && (_Voxels >> pos))
        {
            pos += CountTrailingZeroBits(_Voxels >> pos);

            posAxis = totalBeg + (pos - 1);
            bool transparent = false;
            Voxel voxel = nullptr;

            if(posAxis < totalBeg || posAxis >= totalEnd)
                voxel = _Model->GetVoxel(position);
            else
                voxel = _Chunk.Chunk->find(position);

            if(voxel)
            {
                if(voxel->Material < _Model->Materials.size())
                {
                    const auto &material = _Model->Materials[voxel->Material];
                    transparent = std::fpclassify(material->Transparency) != FP_ZERO;
                }
            }

            if(!transparent)
                mask.Opaque |= ((Config::bitmask_t)1 << pos);
            else
                mask.Transparent |= ((Config::bitmask_t)1 << pos);

            pos++;
        }

        return mask;
    }

    void CFaceMask::GenerateMask(Config::bitmask_t faces, bool backFace, Math::Vec3i position, const Math::Vec3i &_Axis, const SChunkMeta &_Chunk, int _ChunkMask)
    {
        const auto chunk = _Chunk.Chunk;
        auto offset = Config::ChunkSize * (int)backFace;

        Config::bitmask_t pos = 0;
        while ((pos <= (Config::ChunkSize + 2)) && (faces >> pos))
        {
            pos += CountTrailingZeroBits(faces >> pos);
            if(pos >= Config::ChunkSize)
                break;

            position.v[_Axis.x] = pos + _Chunk.TotalBBox.Beg.v[_Axis.x];
            Voxel voxel = chunk->find(position);
            if(!voxel)
            {
                pos++;
                continue;
            }

            uint32_t key = *((uint32_t*)voxel);
            if(GroupAfterMaterial)
                key = voxel->Material;

            auto &mask = m_FacesMasks[pos][key];
            mask.Bits[(position.v[_Axis.z] & Config::InnerChunkMask) + offset] |= (Config::bitmask_t)1 << (position.v[_Axis.y] & _ChunkMask);
            pos++;
        }
    }
} // namespace VCore
