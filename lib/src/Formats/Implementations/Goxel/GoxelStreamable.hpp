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

#ifndef GOXELSTREAMABLE_HPP
#define GOXELSTREAMABLE_HPP

#include <VCore/Formats/Streamable.hpp>
#include <VCore/Misc/FileStream.hpp>
#include <VCore/Meshing/Material.hpp>
#include "GoxelModelParser.hpp"

#include <cstdint>

namespace VCore
{
    class CGoxelStreamable : public IStreamable
    {
        public:
            CGoxelStreamable(
                const std::shared_ptr<IIOHandler> &p_IOHandler,
                const std::string &p_FilePath,
                fast_vector<uint64_t> &&p_BL16Offsets, 
                fast_vector<CMaterial> &&p_Materials, 
                ankerl::unordered_dense::map<Math::Vec3i, fast_vector<ChunkInfo>, Math::Vec3iHasher> &&p_Chunks,
                int p_BeginX, int p_EndX) :
                IStreamable(),
                m_IOHandler(p_IOHandler),
                m_FilePath(p_FilePath),
                m_BL16Offsets(std::move(p_BL16Offsets)),
                m_Materials(std::move(p_Materials)),
                m_Chunks(std::move(p_Chunks)),
                m_BeginX(p_BeginX), m_EndX(p_EndX) {}

            bool SupportsChunkOffloading() const override { return true; }

            bool ReadChunk(const Math::Vec3i &p_Position, CChunk *p_Chunk) override;
        private:
            std::shared_ptr<IIOHandler> m_IOHandler;
            const std::string m_FilePath;
            const fast_vector<uint64_t> m_BL16Offsets;
            const fast_vector<CMaterial> m_Materials;
            const ankerl::unordered_dense::map<Math::Vec3i, fast_vector<ChunkInfo>, Math::Vec3iHasher> m_Chunks;
            int m_BeginX, m_EndX;
    };
} // namespace VCore


#endif