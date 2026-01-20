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

#ifndef GOXELFORMAT_HPP
#define GOXELFORMAT_HPP

#include <VCore/Formats/IVoxelFormat.hpp>
#include <VCore/Misc/fast_vector.hpp>
#include <cstring>
#include "GoxelStreamable.hpp"
#include "GoxelModelParser.hpp"

namespace VCore
{
    /**
     * The goxel file format is similar to the png and MagicaVoxel format.
     * At the moment following chunks are important to V-Core
     * - LAYR: A model in Goxel consists of different layers. Each layer could have it's own material and has it's own chunks
     * - Block: A block contains metadata of a chunk like it's position or where to find the voxel data
     * - BL16: These chunks contains a png file, where each pixel != 0 represents a voxel inside a chunk of 16x16x16
     * - MATE: A chunk with material properties stored in a dictionary
     */
    class CGoxelFormat : public IVoxelFormat
    {
        public:
            CGoxelFormat() = default;        
            ~CGoxelFormat() = default;

        protected:
            void ParseFormat() override;

        private:
            fast_vector<uint64_t> m_BL16Offsets;
            fast_vector<CMaterial> m_Materials;
            ankerl::unordered_dense::map<Math::Vec3i, fast_vector<ChunkInfo>, Math::Vec3iHasher> m_Chunks;

            int m_BeginX, m_EndX;

            /** Reads the file structure. */
            void ReadFile();
            void ProcessMaterial(const SGoxelChunkHeader &p_Chunk);
            void ProcessLayer(const SGoxelChunkHeader &p_Chunk);
            void ProcessBL16(const SGoxelChunkHeader &p_Chunk);
            ankerl::unordered_dense::map<std::string, std::string> ReadDict(const SGoxelChunkHeader &p_Chunk, const size_t p_StartPos);

            void CreateChunk(const Math::Vec3i &p_ChunkPos, VoxelModel &p_Model, CGoxelModelParser &p_Parser);
    };
}

#endif //GOXELFORMAT_HPP