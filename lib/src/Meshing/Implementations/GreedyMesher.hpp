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

#ifndef GREEDYMESHER_HPP
#define GREEDYMESHER_HPP

#include <vector>
#include <VCore/Meshing/IMesher.hpp>
#include "Slicer/Slices.hpp"
#include "../FaceMask.hpp"

namespace VCore
{
    class CGreedyMesher : public IMesher
    {
        public:
            CGreedyMesher(bool _GenerateTexture = false, bool _GenerateSingleChunks = false) : IMesher(), m_GenerateTexture(_GenerateTexture), m_GenerateSingleChunks(_GenerateSingleChunks) {}

            std::vector<SMeshChunk> GenerateChunks(VoxelModel _Mesh, bool _OnlyDirty = false) override;

            virtual ~CGreedyMesher() = default;
        protected:
            bool m_GenerateTexture;
            bool m_GenerateSingleChunks;
            std::pair<const SChunkMeta&, CSliceCollection> GenerateSlicedChunk(VoxelModel m, const SChunkMeta &_Chunk, bool Opaque);

            SMeshChunk GenerateMeshChunk(VoxelModel _Mesh, CSliceCollection &_Collection, const SChunkMeta *_Chunk = nullptr);

            void GenerateQuad(CSliceCollection &result, BITMASK_TYPE faces, CFaceMask::Mask &bits, int width, int depth, bool isFront, const Math::Vec3i &axis, const SChunkMeta &_Chunk, const Voxel _Voxel);
    };
}

#endif