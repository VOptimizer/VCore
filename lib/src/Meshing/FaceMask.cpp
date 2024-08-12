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
    ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<std::string, CFaceMask::Mask>> CFaceMask::Generate(const SChunkMeta &_Chunk, uint8_t _Axis)
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
                uint32_t voxels = _Chunk.Chunk->m_Mask.GetRowFaces(position - subpos, _Axis);
                uint32_t frontFaces = (voxels & (uint32_t)~(voxels << 1)) >> 1;
                uint32_t backFaces = ((voxels & (uint32_t)~(voxels >> 1)) >> 1) & FACE_MASK;

                GenerateMask(frontFaces, false, position, Math::Vec3i(_Axis, axis1, axis2), _Chunk);
                GenerateMask(backFaces, true, position, Math::Vec3i(_Axis, axis1, axis2), _Chunk);
            }
        }

        return std::move(m_FacesMasks);
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
