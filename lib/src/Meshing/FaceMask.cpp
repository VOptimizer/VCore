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
                position.v[axis1] = heightAxis; //- BBox.Beg.v[axis1];
                position.v[axis2] = widthAxis; //- BBox.Beg.v[axis2];

                Math::Vec3i subpos = position & lowerBoundsMask;

                // Math::Vec3i subpos;
                // subpos.v[_Axis] = 0;
                // subpos.v[axis1] = _Chunk.TotalBBox.Beg.v[axis1];
                // subpos.v[axis2] = _Chunk.TotalBBox.Beg.v[axis2];

                // Gets the current "ray" of bits.
                auto voxels = _Chunk.Chunk->m_Mask.GetRowFaces(subpos, _Axis);

                auto mask = GenerateOpaqueMask(_Model, _Chunk, voxels, position, _Axis);

                BITMASK_TYPE frontFaces = (mask.Opaque & (BITMASK_TYPE)~(mask.Opaque << 1)) >> 1;
                BITMASK_TYPE backFaces = ((mask.Opaque & (BITMASK_TYPE)~(mask.Opaque >> 1)) >> 1) & FACE_MASK;

                if(mask.Opaque)
                {
                    GenerateMask(frontFaces, false, position, Math::Vec3i(_Axis, axis1, axis2), _Chunk);
                    GenerateMask(backFaces, true, position, Math::Vec3i(_Axis, axis1, axis2), _Chunk);
                }

                if(mask.Transparent)
                {
                    BITMASK_TYPE transparentFrontFaces = (mask.Transparent & (BITMASK_TYPE)~(mask.Transparent << 1)) >> 1;
                    BITMASK_TYPE transparentBackFaces = ((mask.Transparent & (BITMASK_TYPE)~(mask.Transparent >> 1)) >> 1) & FACE_MASK;

                    GenerateMask(transparentFrontFaces & ~(frontFaces << 1), false, position, Math::Vec3i(_Axis, axis1, axis2), _Chunk);
                    GenerateMask(transparentBackFaces & ~(backFaces >> 1), true, position, Math::Vec3i(_Axis, axis1, axis2), _Chunk);
                }
            }
        }

        return std::move(m_FacesMasks);
    }

    ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<uint32_t, CFaceMask::Mask>> CFaceMask::GenerateChunkBoundary(const VoxelModel &_Model, const SChunkMeta &_Chunk, uint8_t _Axis)
    {
        const CBBox &BBox = _Chunk.InnerBBox;

        // This logic calculates the index of one of the three other axis.
        int axis1 = (_Axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
        int axis2 = (_Axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y

        const static uint32_t lowerBoundsMask = (CHUNK_SIZE - 1);
        const static uint32_t chunkMask = ~lowerBoundsMask;
        ankerl::unordered_dense::map<Math::Vec3i, const CChunk*, Math::Vec3iHasher> chunks = {{_Chunk.TotalBBox.Beg, _Chunk.Chunk}};

        auto beginPosX = BBox.Beg.v[axis2];
        auto beginPosY = BBox.Beg.v[axis1];
        auto endPosX = BBox.End.v[axis2];
        auto endPosY = BBox.End.v[axis1];

        if((beginPosX & lowerBoundsMask) == 0)
        {
            beginPosX--;
            auto pos = _Chunk.TotalBBox.Beg;
            pos.v[_Axis] = 0;
            pos.v[axis2] = beginPosX;
            chunks[pos] = _Model->GetVoxels().getChunk(pos);
        }

        if((beginPosY & lowerBoundsMask) == 0)
        {
            beginPosY--;
            auto pos = _Chunk.TotalBBox.Beg;
            pos.v[_Axis] = 0;
            pos.v[axis1] = beginPosY;
            chunks[pos] = _Model->GetVoxels().getChunk(pos);

            if((BBox.Beg.v[axis2] & lowerBoundsMask) == 0)
            {
                pos.v[axis2] = beginPosX;
                chunks[pos] = _Model->GetVoxels().getChunk(pos);
            }
        }

        if((endPosX & lowerBoundsMask) == (CHUNK_SIZE - 1))
        {
            endPosX++;
            auto pos = _Chunk.TotalBBox.Beg;
            pos.v[_Axis] = 0;
            pos.v[axis2] = endPosX;
            chunks[pos] = _Model->GetVoxels().getChunk(pos);

            if((BBox.Beg.v[axis1] & lowerBoundsMask) == 0)
            {
                pos.v[axis1] = beginPosY;
                chunks[pos] = _Model->GetVoxels().getChunk(pos);
            }
        }

        if((endPosY & lowerBoundsMask) == (CHUNK_SIZE - 1))
        {
            endPosY--;
            auto pos = _Chunk.TotalBBox.Beg;
            pos.v[_Axis] = 0;
            pos.v[axis1] = endPosY;
            chunks[pos] = _Model->GetVoxels().getChunk(pos);

            if((BBox.Beg.v[axis2] & lowerBoundsMask) == 0)
            {
                pos.v[axis2] = beginPosX;
                chunks[pos] = _Model->GetVoxels().getChunk(pos);
            }

            if((BBox.End.v[axis2] & lowerBoundsMask) == (CHUNK_SIZE - 1))
            {
                pos.v[axis2] = endPosX;
                chunks[pos] = _Model->GetVoxels().getChunk(pos);
            }
        }

        for (int heightAxis = beginPosY; heightAxis <= endPosY; heightAxis++)
        {
            for (int widthAxis = beginPosX; widthAxis <= endPosX; widthAxis++)
            {
                Math::Vec3i position;
                position.v[_Axis] = 0;
                position.v[axis1] = heightAxis; //- BBox.Beg.v[axis1];
                position.v[axis2] = widthAxis; //- BBox.Beg.v[axis2];

                auto chunk = chunks[position & chunkMask];
                if(!chunk)
                    continue;

                Math::Vec3i subpos = position & lowerBoundsMask;

                // Math::Vec3i subpos;
                // subpos.v[_Axis] = 0;
                // subpos.v[axis1] = _Chunk.TotalBBox.Beg.v[axis1];
                // subpos.v[axis2] = _Chunk.TotalBBox.Beg.v[axis2];

                // Gets the current "ray" of bits.
                auto voxels = chunk->m_Mask.GetRowFaces(subpos, _Axis);

                CBBox totalBBox = _Chunk.TotalBBox;
                if(chunk != _Chunk.Chunk)
                {
                    totalBBox.Beg.v[axis1] = beginPosY;
                    totalBBox.Beg.v[axis2] = beginPosX;

                    totalBBox.End.v[axis1] = beginPosY + CHUNK_SIZE;
                    totalBBox.End.v[axis2] = beginPosX + 1;
                }

                auto mask = GenerateOpaqueMask(_Model, voxels, position, _Axis, totalBBox, chunk);

                // auto mask = GenerateOpaqueMask(_Model, _Chunk, voxels, position, _Axis);

                BITMASK_TYPE frontFaces = (mask.Opaque & (BITMASK_TYPE)~(mask.Opaque << 1)) >> 1;
                BITMASK_TYPE backFaces = ((mask.Opaque & (BITMASK_TYPE)~(mask.Opaque >> 1)) >> 1) & FACE_MASK;

                if(mask.Opaque)
                {
                    GenerateMask(frontFaces, false, position, Math::Vec3i(_Axis, axis1, axis2), totalBBox, chunk);
                    GenerateMask(backFaces, true, position, Math::Vec3i(_Axis, axis1, axis2), totalBBox, chunk);
                }

                if(mask.Transparent)
                {
                    BITMASK_TYPE transparentFrontFaces = (mask.Transparent & (BITMASK_TYPE)~(mask.Transparent << 1)) >> 1;
                    BITMASK_TYPE transparentBackFaces = ((mask.Transparent & (BITMASK_TYPE)~(mask.Transparent >> 1)) >> 1) & FACE_MASK;

                    GenerateMask(transparentFrontFaces & ~(frontFaces << 1), false, position, Math::Vec3i(_Axis, axis1, axis2), totalBBox, chunk);
                    GenerateMask(transparentBackFaces & ~(backFaces >> 1), true, position, Math::Vec3i(_Axis, axis1, axis2), totalBBox, chunk);
                }
            }
        }

        return std::move(m_FacesMasks);
    }

    CFaceMask::OpaqueMask CFaceMask::GenerateOpaqueMask(const VoxelModel &_Model, BITMASK_TYPE _Voxels, Math::Vec3i position, uint8_t _Axis, const CBBox &_TotalBBox, const CChunk *_Chunk)
    {
        OpaqueMask mask;

        auto totalBeg = _TotalBBox.Beg.v[_Axis];
        auto totalEnd = _TotalBBox.End.v[_Axis];
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
                voxel = _Chunk->find(position);

            if(voxel)
            {
                if(voxel->Material < _Model->Materials.size())
                {
                    const auto &material = _Model->Materials[voxel->Material];
                    transparent = std::fpclassify(material->Transparency) == FP_ZERO;
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

    void CFaceMask::GenerateMask(BITMASK_TYPE faces, bool backFace, Math::Vec3i position, const Math::Vec3i &_Axis, const CBBox &_TotalBBox, const CChunk *_Chunk)
    {
        const auto chunk = _Chunk;

        BITMASK_TYPE pos = 0;
        while ((pos <= (CHUNK_SIZE + 2)) && (faces >> pos))
        {
            pos += CountTrailingZeroBits(faces >> pos);
            if(pos >= CHUNK_SIZE)
                break;

            position.v[_Axis.x] = pos + _TotalBBox.Beg.v[_Axis.x];
            auto voxel = chunk->find(position); // ->GetVoxel(position);
            if(!voxel)
            {
                pos++;
                continue;
            }

            // std::string key = std::to_string(voxel->Material) + "_" + std::to_string(voxel->Color);
            auto &mask = m_FacesMasks[pos][*((uint32_t*)voxel)];

            mask.Bits[position.v[_Axis.z] - _TotalBBox.Beg.v[_Axis.z] + (CHUNK_SIZE + 2) * (int)backFace] |= (BITMASK_TYPE)1 << (position.v[_Axis.y] & (CHUNK_SIZE - 1));
            pos++;
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
                    transparent = std::fpclassify(material->Transparency) == FP_ZERO;
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

    void CFaceMask::GenerateMask(BITMASK_TYPE faces, bool backFace, Math::Vec3i position, const Math::Vec3i &_Axis, const SChunkMeta &_Chunk)
    {
        const auto chunk = _Chunk.Chunk;

        BITMASK_TYPE pos = 0;
        while ((pos <= (CHUNK_SIZE + 2)) && (faces >> pos))
        {
            pos += CountTrailingZeroBits(faces >> pos);
            if(pos >= CHUNK_SIZE)
                break;

            position.v[_Axis.x] = pos + _Chunk.TotalBBox.Beg.v[_Axis.x];
            auto voxel = chunk->find(position); // ->GetVoxel(position);
            if(!voxel)
            {
                pos++;
                continue;
            }

            // std::string key = std::to_string(voxel->Material) + "_" + std::to_string(voxel->Color);
            auto &mask = m_FacesMasks[pos][*((uint32_t*)voxel)];

            mask.Bits[position.v[_Axis.z] - _Chunk.TotalBBox.Beg.v[_Axis.z] + CHUNK_SIZE * (int)backFace] |= (BITMASK_TYPE)1 << (position.v[_Axis.y] & (CHUNK_SIZE - 1));
            pos++;
        }
    }
} // namespace VCore
