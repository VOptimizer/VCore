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

#ifndef CELLCACHE_HPP
#define CELLCACHE_HPP

#include <VCore/Voxel/Voxel.hpp>
#include <VCore/VConfig.hpp>
#include <VCore/Math/Vector.hpp>
#include <cstdint>
#include <cstring>

namespace VCore 
{
    class CCellCache
    {
        public:
            CCellCache()
            {
                Reset();
            }

            void Reset()
            {
                std::memset(m_Cells, 0xFF, sizeof(m_Cells));
            }

            void CacheVertex(const Math::Vec3i &p_Pos, uint8_t p_Vertex, uint32_t p_Index, const CVoxel &p_Voxel)
            {
                auto idx = GetIndex(p_Pos);
                m_Cells[idx].VertexIndex[p_Vertex] = p_Index;
                m_Cells[idx].Voxel = p_Voxel;
            }

            bool HasCachedVertex(const Math::Vec3i &p_Pos, uint8_t p_Vertex, const CVoxel &p_Voxel)
            {
                auto idx = GetIndex(p_Pos);
                return m_Cells[idx].VertexIndex[p_Vertex] != 0xFFFFFFFF && m_Cells[idx].Voxel == p_Voxel;
            }

            uint32_t GetCachedVertex(const Math::Vec3i &p_Pos, uint8_t p_Vertex)
            {
                auto idx = GetIndex(p_Pos);
                return m_Cells[idx].VertexIndex[p_Vertex];
            }
        private:
            struct Cell
            {
                uint32_t VertexIndex[4];
                CVoxel Voxel;
            };

            constexpr static auto CHUNK_SIZE = Config::ChunkSize + 1;

            uint32_t GetIndex(const Math::Vec3i &p_Pos)
            {
                return ((p_Pos.z & 0x1) * (CHUNK_SIZE * CHUNK_SIZE)) + p_Pos.x + CHUNK_SIZE * p_Pos.y;
            }

            Cell m_Cells[2 * (CHUNK_SIZE * CHUNK_SIZE)];
    };
}

#endif