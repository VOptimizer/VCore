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

namespace VCore
{
    class CGreedyMesher : public IMesher
    {
        public:
            CGreedyMesher(bool _GenerateTexture = false) : IMesher(), m_GenerateTexture(_GenerateTexture) {}

            std::vector<SMeshChunk> GenerateChunks(VoxelModel _Mesh, bool _OnlyDirty = false) override;

            virtual ~CGreedyMesher() = default;
        protected:
            struct Mask
            {
                uint32_t Bits[CHUNK_SIZE * 2];
            };

            bool m_GenerateTexture;
            CSliceCollection GenerateSlicedChunk(VoxelModel m, const SChunkMeta &_Chunk, bool Opaque);

            void GenerateMask(uint32_t faces, bool backFace, ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<std::string, Mask>> &mask, Math::Vec3i position, const Math::Vec3i &_Axis, const SChunkMeta &_Chunk);

            void GenerateQuad(CSliceCollection &result, uint32_t faces, Mask &bits, int width, int depth, bool isFront, const Math::Vec3i &axis, const SChunkMeta &_Chunk, const std::vector<std::string> &parts);
            // SMeshChunk GenerateMeshChunk(VoxelModel m, const SChunkMeta &_Chunk, bool Opaque) override;
    };
}

#endif