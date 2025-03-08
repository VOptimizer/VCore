/*
 * MIT License
 *
 * Copyright (c) 2025 Christian Tost
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

#ifndef GOXELMODELPARSER_HPP
#define GOXELMODELPARSER_HPP

#include <VCore/Misc/FileStream.hpp>
#include <VCore/Misc/fast_vector.hpp>
#include <VCore/Misc/unordered_dense.h>
#include <VCore/Meshing/Material.hpp>
#include <VCore/Voxel/Storage/Chunk.hpp>

#include <stdint.h>

namespace VCore
{
    struct ChunkInfo
    {
        uint32_t BL16Index;
        uint8_t Material;
    };

    struct SGoxelChunkHeader
    {
        char Type[4];
        uint32_t Size;   // Datasize
    };

    class CGoxelModelParser
    {
        public:
            CGoxelModelParser(
                IFileStream *p_Stream,
                const fast_vector<uint64_t> &p_BL16Offsets,
                const fast_vector<CMaterial> &p_Materials,
                const ankerl::unordered_dense::map<Math::Vec3i, fast_vector<ChunkInfo>, Math::Vec3iHasher> &p_Chunks,
                int p_BeginX, int p_EndX
            ) : m_Stream(p_Stream), m_BL16Offsets(p_BL16Offsets), m_Materials(p_Materials), m_Chunks(p_Chunks), m_BeginX(p_BeginX), m_EndX(p_EndX) {}
            
            void FillChunk(const Math::Vec3i &p_Position, CChunk *p_Chunk);
            
            ~CGoxelModelParser() {}
        private:
            IFileStream *m_Stream;
            const fast_vector<uint64_t> &m_BL16Offsets;
            const fast_vector<CMaterial> &m_Materials;
            const ankerl::unordered_dense::map<Math::Vec3i, fast_vector<ChunkInfo>, Math::Vec3iHasher> &m_Chunks;
            int m_BeginX, m_EndX;
    };
} // namespace VCore

#endif