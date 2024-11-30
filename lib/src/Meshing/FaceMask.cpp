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
#include "Implementations/Simd.hpp"
#include "../Misc/Helper.hpp"
#include <cmath>

namespace VCore
{
    ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<uint32_t, CFaceMask::Mask>> CFaceMask::Generate(const VoxelModel &_Model, const SChunkMeta &_Chunk, uint8_t _Axis)
    {
        InternalGenerate(_Model, _Chunk, _Axis, Config::InnerChunkMask);

        // const CBBox &BBox = _Chunk.InnerBBox;
        // const auto relSubBBoxBeg = BBox.Beg & Config::InnerChunkMask;
        // const auto relSubBBoxEnd = BBox.End & Config::InnerChunkMask;

        // // This logic calculates the index of one of the three other axis.
        // const int axis1 = (_Axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
        // const int axis2 = (_Axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y

        // const Simd::Simdi<128> notInstanziatedMask(-1);

        // for (int xpos = relSubBBoxBeg.v[axis2]; xpos <= relSubBBoxEnd.v[axis2]; xpos++)
        // {
        //     for (int zpos = relSubBBoxBeg.v[_Axis]; zpos <= relSubBBoxEnd.v[_Axis]; zpos++)
        //     {
        //         Math::Vec3i position;
        //         position.v[_Axis] = zpos;
        //         // position.v[axis1] = 0;
        //         position.v[axis2] = xpos;

        //         uint32_t voxelCol = 0;
        //         VoxelSpan span;
        //         _Chunk.Chunk->GetVoxelSpan(span, position, _Axis);
        //         for (int i = 0; i < Config::ChunkSize; i += 4)
        //         {
        //             Simd::Simdi<128> voxels((int*)(span.Voxels + i), 4);
        //             voxelCol |= (voxels & notInstanziatedMask).MoveMask() << i;
        //         }
        //     }
        // }

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

    void FillVoxelBits(int *_Voxels, const IChunk *_Chunk, const Math::Vec3i &_Axis, const Math::Vec3i &_Position, const int _Count)
    {
        Math::Vec3i subpos = _Position & Config::InnerChunkMask;
        for (int i = 0; i < _Count; i++)
        {
            _Voxels[i] = (_Chunk->m_Mask.GetRowFaces(subpos, _Axis.y) >> 1) & 0xFFFFFFFF;
            subpos.v[_Axis.z]++;
        }
    }

    void CFaceMask::InternalGenerate(const VoxelModel &_Model, const SChunkMeta &_Chunk, uint8_t _Axis, int _ChunkMask)
    {
        const CBBox &BBox = _Chunk.InnerBBox;

        // This logic calculates the index of one of the three other axis.
        int axis1 = (_Axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
        int axis2 = (_Axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y

        // for (int heightAxis = BBox.Beg.v[axis1]; heightAxis <= BBox.End.v[axis1]; heightAxis++)
        for (int depthAxis = BBox.Beg.v[_Axis]; depthAxis <= BBox.End.v[_Axis]; depthAxis++)
        {
            for (int widthAxis = BBox.Beg.v[axis2]; widthAxis <= BBox.End.v[axis2]; widthAxis += 4)
            {
                Math::Vec3i position;
                position.v[_Axis] = depthAxis;
                position.v[axis1] = BBox.Beg.v[axis1]; //heightAxis;
                position.v[axis2] = widthAxis;

                Math::Vec3i subpos = position & Config::InnerChunkMask;
                const Math::Vec3i axis(_Axis, axis1, axis2);

                int beforeVoxels[4] = {};
                int voxels[4] = {};
                int afterVoxels[4] = {};

                const int count = ((BBox.End.v[axis2] - widthAxis) >= 4) ? 4 : ((BBox.End.v[axis2] - widthAxis) + 1);
                FillVoxelBits(voxels, _Chunk.Chunk, axis, position, count);

                position.v[_Axis]--;
                auto chunk = _Chunk.Chunk;

                if((position.v[_Axis] & Config::InnerChunkMask) >= Config::ChunkSize - 1)
                    chunk = _Model->GetVoxels().getChunk(position);

                if(chunk)
                    FillVoxelBits(beforeVoxels, chunk, axis, position, count);

                position.v[_Axis] += 2;
                chunk = _Chunk.Chunk;

                if((position.v[_Axis] & Config::InnerChunkMask) == 0)
                    chunk = _Model->GetVoxels().getChunk(position);

                if(chunk)
                    FillVoxelBits(afterVoxels, chunk, axis, position, count);

                Simd::Simdi<128> beforeVoxelsSimd(beforeVoxels, 4);
                Simd::Simdi<128> voxelsSimd(voxels, 4);
                Simd::Simdi<128> afterVoxelsSimd(afterVoxels, 4);

                auto &mask = m_FacesMasks[subpos.v[_Axis]][0];

                int buf[4] = {};
                ((beforeVoxelsSimd & voxelsSimd) ^ voxelsSimd).Store(buf, 4);
                for (int i = 0; i < 4; i++)
                    mask.Bits[subpos.v[axis2] + i] = (unsigned int)buf[i];
                
                ((afterVoxelsSimd & voxelsSimd) ^ voxelsSimd).Store(buf, 4);
                for (int i = 0; i < 4; i++)
                    mask.Bits[subpos.v[axis2] + i + Config::ChunkSize] = (unsigned int)buf[i];

                // for (int i = 0; i < ((BBox.End.v[axis2] - widthAxis) > 4) ? 4 : (BBox.End.v[axis2] - widthAxis); i++)
                // {
                //     voxels[i] = (_Chunk.Chunk->m_Mask.GetRowFaces(subpos, axis1) >> 1) & 0xFFFFFFFF;
                //     subpos.v[axis1]++;
                // }
            
                

                // auto voxels = _Chunk.Chunk->m_Mask.GetRowFaces(subpos, axis1);

                // Gets the current "ray" of bits.
                // auto voxels = _Chunk.Chunk->m_Mask.GetRowFaces(subpos, _Axis);
                // if(!voxels)
                //     continue;

                // // Splits the bits into opaque and transparent ones.
                // auto mask = GenerateOpaqueMask(_Model, _Chunk, voxels, position, _Axis);

                // // Generates a mask of all in a "ray" visible faces!
                // Config::bitmask_t frontFaces = (mask.Opaque & (Config::bitmask_t)~(mask.Opaque << 1)) >> 1;
                // Config::bitmask_t backFaces = ((mask.Opaque & (Config::bitmask_t)~(mask.Opaque >> 1)) >> 1) & Config::FaceMask;

                // const Math::Vec3i axis(_Axis, axis1, axis2);
                // if(mask.Opaque)
                // {
                //     GenerateMask(frontFaces, false, position, axis, _Chunk, _ChunkMask);
                //     GenerateMask(backFaces, true, position, axis, _Chunk, _ChunkMask);
                // }

                // if(mask.Transparent)
                // {
                //     Config::bitmask_t transparentFrontFaces = (mask.Transparent & (Config::bitmask_t)~(mask.Transparent << 1)) >> 1;
                //     Config::bitmask_t transparentBackFaces = ((mask.Transparent & (Config::bitmask_t)~(mask.Transparent >> 1)) >> 1) & Config::FaceMask;

                //     GenerateMask(transparentFrontFaces & ~(frontFaces << 1), false, position, axis, _Chunk, _ChunkMask);
                //     GenerateMask(transparentBackFaces & ~(backFaces >> 1), true, position, axis, _Chunk, _ChunkMask);
                // }
            }
        }
    }

    CFaceMask::OpaqueMask CFaceMask::GenerateOpaqueMask(const VoxelModel &_Model, const SChunkMeta &_Chunk, Config::bitmask_t _Voxels, Math::Vec3i position, uint8_t _Axis)
    {
        OpaqueMask mask;

        auto totalBeg = _Chunk.TotalBBox.Beg.v[_Axis];
        auto totalEnd = _Chunk.TotalBBox.End.v[_Axis];
        auto &posAxis = position.v[_Axis];

        mask.Opaque = _Voxels;
        return mask;

        Config::bitmask_t pos = 0;
        while ((pos <= (Config::ChunkSize + 2)) && (_Voxels >> pos))
        {
            pos += CountTrailingZeroBits(_Voxels >> pos);

            posAxis = totalBeg + (pos - 1);
            bool transparent = false;
            CVoxel voxel;

            if(posAxis < totalBeg || posAxis >= totalEnd)
                voxel = _Model->GetVoxel(position);
            else
                voxel = _Chunk.Chunk->find(position);

            if(voxel.IsInstantiated())
            {
                if(voxel.Material < _Model->Materials.size())
                {
                    const auto &material = _Model->Materials[voxel.Material];
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

        auto bitIdx = (position.v[_Axis.z] & Config::InnerChunkMask) + offset;
        auto bitmask = (Config::bitmask_t)1 << (position.v[_Axis.y] & _ChunkMask);

        int pos = 0;
        while ((pos <= (Config::ChunkSize + 2)) && (faces >> pos))
        {
            pos += CountTrailingZeroBits(faces >> pos);
            if(pos >= Config::ChunkSize)
                break;

            position.v[_Axis.x] = pos + _Chunk.TotalBBox.Beg.v[_Axis.x];
            auto voxel = chunk->find(position);
            if(!voxel.IsInstantiated())
            {
                pos++;
                continue;
            }

            uint32_t key = *((uint32_t*)&voxel);
            if(GroupAfterMaterial)
                key = voxel.Material;

            auto &mask = m_FacesMasks[pos][key];
            mask.Bits[bitIdx] |= bitmask;
            pos++;
        }
    }
} // namespace VCore
