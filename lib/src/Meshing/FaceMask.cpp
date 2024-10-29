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
        InternalGenerate(_Model, _Chunk, _Axis, (CHUNK_SIZE - 1));
        return std::move(m_FacesMasks);
    }

    ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<uint32_t, CFaceMask::Mask>> CFaceMask::Generate(const VoxelModel &_Model, const Math::Vec3i &_Position, uint8_t _Axis)
    {
        auto chunkPos = GetChunkpos(_Position);
        Math::Vec3iHasher hasher;

        SChunkMeta meta;
        meta.Chunk = _Model->GetVoxels().getChunk(chunkPos);
        if(meta.Chunk)
        {
            meta.UniqueId = hasher(chunkPos);
            meta.TotalBBox = CBBox(chunkPos, chunkPos + Math::Vec3i(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE));
            meta.InnerBBox = meta.Chunk->inner_bbox(chunkPos);
            InternalGenerate(_Model, meta, _Axis, ((CHUNK_SIZE << 1) - 1));
        }

        Math::Vec3i add;
        add.v[(_Axis + 1) % 3] = CHUNK_SIZE;

        chunkPos = GetChunkpos(_Position + add);
        meta.Chunk = _Model->GetVoxels().getChunk(chunkPos);
        if(meta.Chunk)
        {
            meta.UniqueId = hasher(chunkPos);
            meta.TotalBBox = CBBox(chunkPos, chunkPos + Math::Vec3i(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE));
            meta.InnerBBox = meta.Chunk->inner_bbox(chunkPos);
            InternalGenerate(_Model, meta, _Axis, ((CHUNK_SIZE << 1) - 1));
        }

        return std::move(m_FacesMasks);
    }

    void CFaceMask::InternalGenerate(const VoxelModel &_Model, const SChunkMeta &_Chunk, uint8_t _Axis, int _ChunkMask)
    {
        const CBBox &BBox = _Chunk.InnerBBox;

        // This logic calculates the index of one of the three other axis.
        int axis1 = (_Axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
        int axis2 = (_Axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y

        const static uint32_t lowerBoundsMask = (CHUNK_SIZE - 1);

        for (int heightAxis = BBox.Beg.v[axis1]; heightAxis <= BBox.End.v[axis1]; heightAxis++)
        {
            for (int widthAxis = BBox.Beg.v[axis2]; widthAxis <= BBox.End.v[axis2]; widthAxis++)
            {
                Math::Vec3i position;
                position.v[_Axis] = 0;
                position.v[axis1] = heightAxis;
                position.v[axis2] = widthAxis;

                Math::Vec3i subpos = position & lowerBoundsMask;

                // Gets the current "ray" of bits.
                auto voxels = _Chunk.Chunk->m_Mask.GetRowFaces(subpos, _Axis);

                // Splits the bits into opaque and transparent ones.
                auto mask = GenerateOpaqueMask(_Model, _Chunk, voxels, position, _Axis);

                // Generates a mask of all in a "ray" visible faces!
                BITMASK_TYPE frontFaces = (mask.Opaque & (BITMASK_TYPE)~(mask.Opaque << 1)) >> 1;
                BITMASK_TYPE backFaces = ((mask.Opaque & (BITMASK_TYPE)~(mask.Opaque >> 1)) >> 1) & FACE_MASK;

                if(mask.Opaque)
                {
                    GenerateMask(frontFaces, false, position, Math::Vec3i(_Axis, axis1, axis2), _Chunk, _ChunkMask);
                    GenerateMask(backFaces, true, position, Math::Vec3i(_Axis, axis1, axis2), _Chunk, _ChunkMask);
                }

                if(mask.Transparent)
                {
                    BITMASK_TYPE transparentFrontFaces = (mask.Transparent & (BITMASK_TYPE)~(mask.Transparent << 1)) >> 1;
                    BITMASK_TYPE transparentBackFaces = ((mask.Transparent & (BITMASK_TYPE)~(mask.Transparent >> 1)) >> 1) & FACE_MASK;

                    GenerateMask(transparentFrontFaces & ~(frontFaces << 1), false, position, Math::Vec3i(_Axis, axis1, axis2), _Chunk, _ChunkMask);
                    GenerateMask(transparentBackFaces & ~(backFaces >> 1), true, position, Math::Vec3i(_Axis, axis1, axis2), _Chunk, _ChunkMask);
                }
            }
        }
    }

    CFaceMask::OpaqueMask CFaceMask::GenerateOpaqueMask(const VoxelModel &_Model, const SChunkMeta &_Chunk, BITMASK_TYPE _Voxels, Math::Vec3i position, uint8_t _Axis)
    {
        OpaqueMask mask;

        auto totalBeg = _Chunk.TotalBBox.Beg.v[_Axis];
        auto totalEnd = _Chunk.TotalBBox.End.v[_Axis];
        auto &posAxis = position.v[_Axis];

        BITMASK_TYPE pos = 0;
        while ((pos <= (CHUNK_SIZE + 2)) && (_Voxels >> pos))
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
                mask.Opaque |= ((BITMASK_TYPE)1 << pos);
            else
                mask.Transparent |= ((BITMASK_TYPE)1 << pos);

            pos++;
        }

        return mask;
    }

    void CFaceMask::GenerateMask(BITMASK_TYPE faces, bool backFace, Math::Vec3i position, const Math::Vec3i &_Axis, const SChunkMeta &_Chunk, int _ChunkMask)
    {
        const auto chunk = _Chunk.Chunk;

        BITMASK_TYPE pos = 0;
        while ((pos <= (CHUNK_SIZE + 2)) && (faces >> pos))
        {
            pos += CountTrailingZeroBits(faces >> pos);
            if(pos >= CHUNK_SIZE)
                break;

            position.v[_Axis.x] = pos + _Chunk.TotalBBox.Beg.v[_Axis.x];
            auto voxel = chunk->find(position);
            if(!voxel)
            {
                pos++;
                continue;
            }

            auto &mask = m_FacesMasks[pos][*((uint32_t*)voxel)];
            mask.Bits[position.v[_Axis.z] - _Chunk.TotalBBox.Beg.v[_Axis.z] + CHUNK_SIZE * (int)backFace] |= (BITMASK_TYPE)1 << (position.v[_Axis.y] & _ChunkMask);
            pos++;
        }
    }
} // namespace VCore
