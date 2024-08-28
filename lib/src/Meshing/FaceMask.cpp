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

namespace VCore
{
    ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<std::string, CFaceMask::Mask>> CFaceMask::Generate(const VoxelModel &_Model, const SChunkMeta &_Chunk, uint8_t _Axis)
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
                position.v[axis1] = heightAxis; //- BBox.Beg.v[axis1];
                position.v[axis2] = widthAxis; //- BBox.Beg.v[axis2];

                Math::Vec3i subpos;
                subpos.v[_Axis] = 0;
                subpos.v[axis1] = _Chunk.TotalBBox.Beg.v[axis1];
                subpos.v[axis2] = _Chunk.TotalBBox.Beg.v[axis2];

                // Gets the current "ray" of bits.
                auto voxels = _Chunk.Chunk->m_Mask.GetRowFaces(position - subpos, _Axis);

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

    CFaceMask::OpaqueMask CFaceMask::GenerateOpaqueMask(const VoxelModel &_Model, const SChunkMeta &_Chunk, BITMASK_TYPE _Voxels, Math::Vec3i position, uint8_t _Axis)
    {
        OpaqueMask mask;

        int pos = CountTrailingZeroBits(_Voxels);
        while ((pos <= (CHUNK_SIZE + 2)) && (_Voxels >> pos))
        {
            position.v[_Axis] = _Chunk.TotalBBox.Beg.v[_Axis] + (pos - 1);
            bool transparent = false;
            Voxel voxel = nullptr;

            if(position.v[_Axis] < _Chunk.TotalBBox.Beg.v[_Axis] || position.v[_Axis] >= _Chunk.TotalBBox.End.v[_Axis] )
                voxel = _Model->GetVoxel(position);
            else
                voxel = _Chunk.Chunk->find(position, CBBox(_Chunk.TotalBBox.Beg, _Chunk.TotalBBox.End - _Chunk.TotalBBox.Beg));

            if(voxel)
            {
                if(voxel->Material < _Model->Materials.size())
                {
                    auto material = _Model->Materials[voxel->Material];
                    transparent = material->Transparency != 0.0;
                }
            }

            if(!transparent)
                mask.Opaque |= (1 << pos);
            else
                mask.Transparent |= (1 << pos);

            pos++;
            pos += CountTrailingZeroBits(_Voxels >> pos);
        }

        return mask;
    }

    void CFaceMask::GenerateMask(BITMASK_TYPE faces, bool backFace, Math::Vec3i position, const Math::Vec3i &_Axis, const SChunkMeta &_Chunk)
    {
        int pos = 0;
        while ((pos <= (CHUNK_SIZE + 2)) && (faces >> pos))
        {
            pos += CountTrailingZeroBits(faces >> pos);
            if(pos >= CHUNK_SIZE)
                break;

            position.v[_Axis.x] = pos + _Chunk.TotalBBox.Beg.v[_Axis.x];
            auto voxel = _Chunk.Chunk->find(position, CBBox(_Chunk.TotalBBox.Beg, Math::Vec3i(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE))); // ->GetVoxel(position);
            if(!voxel)
            {
                pos++;
                continue;
            }

            std::string key = std::to_string(voxel->Material) + "_" + std::to_string(voxel->Color);
            auto &mask = m_FacesMasks[pos][key];

            mask.Bits[position.v[_Axis.z] - _Chunk.TotalBBox.Beg.v[_Axis.z] + CHUNK_SIZE * (int)backFace] |= 1 << (position.v[_Axis.y] - _Chunk.TotalBBox.Beg.v[_Axis.y]);
            pos++;
        }
    }
} // namespace VCore
