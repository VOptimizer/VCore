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

#include <stdint.h>

namespace VCore
{
    class CGoxelStreamable : public IStreamable
    {
        public:
            CGoxelStreamable(
                const std::shared_ptr<IIOHandler> &_IOHandler,
                const std::string &_FilePath,
                fast_vector<uint64_t> &&_BL16Offsets, 
                fast_vector<CMaterial> &&_Materials, 
                ankerl::unordered_dense::map<Math::Vec3i, fast_vector<ChunkInfo>, Math::Vec3iHasher> &&_Chunks,
                int _BeginX, int _EndX) :
                IStreamable(),
                m_IOHandler(_IOHandler),
                m_FilePath(_FilePath),
                m_BL16Offsets(std::move(_BL16Offsets)),
                m_Materials(std::move(_Materials)),
                m_Chunks(std::move(_Chunks)),
                m_BeginX(_BeginX), m_EndX(_EndX) {}

            bool SupportsChunkOffloading() const override { return true; }

            bool ReadChunk(const Math::Vec3i &_Position, CChunk *_Chunk) override;
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