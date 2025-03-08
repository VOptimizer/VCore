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

#include "GoxelModelParser.hpp"
#include <stb_image.h>
#include <VCore/Meshing/MaterialManager.hpp>

namespace VCore
{
    constexpr static uint32_t ChunkIterationSize = Config::ChunkSize < 16 ? Config::ChunkSize : 16;
    constexpr static uint32_t GoxelChunkSize = 16;

    // Goxel uses 16x16x16 chunk sizes, V-Core is configureabel and can have chunks sizes 8, 16, 32, and 64.
    inline Math::Vec3i GetGoxelChunkpos16(const Math::Vec3i &p_Position)
    {
        return p_Position & ~(GoxelChunkSize - 1);
    }

    void CGoxelModelParser::FillChunk(const Math::Vec3i &p_Position, CChunk *p_Chunk)
    {
        auto goxelPos = GetChunkpos(Math::Vec3i(abs(m_EndX - m_BeginX) - 1, 0, 0) - (p_Position - Math::Vec3i(m_BeginX, 0, 0)) + Math::Vec3i(m_BeginX, 0, 0));
        auto innerStartPos = p_Position & (GoxelChunkSize - 1);

        // TODO: 8 Chunk Path
        for (int cx = goxelPos.x; cx < static_cast<int>(goxelPos.x + Config::ChunkSize); cx += GoxelChunkSize)
        {
            for (int cy = goxelPos.y; cy < static_cast<int>(goxelPos.y + Config::ChunkSize); cy += GoxelChunkSize)
            {
                for (int cz = goxelPos.z; cz < static_cast<int>(goxelPos.z + Config::ChunkSize); cz += GoxelChunkSize)
                {
                    auto it = m_Chunks.find(Math::Vec3i((ChunkIterationSize - cx - goxelPos.x) + goxelPos.x, cy, cz));
                    if(it != m_Chunks.end())
                    {
                        for (auto &&chunkinfo : it->second)
                        {
                            if(chunkinfo.BL16Index < m_BL16Offsets.size())
                            {
                                m_Stream->Seek(m_BL16Offsets[chunkinfo.BL16Index], VCore::SeekOrigin::BEG);
                                SGoxelChunkHeader chunk = m_Stream->Read<SGoxelChunkHeader>();
            
                                stbi_uc *pngData = new stbi_uc[chunk.Size];
                                m_Stream->Read((char*)pngData, chunk.Size);
            
                                int w, h, c;
                                uint32_t *imgData = (uint32_t*)stbi_load_from_memory(pngData, chunk.Size, &w, &h, &c, 4);
                                delete[] pngData;
            
                                for (uint32_t z = innerStartPos.z; z < ChunkIterationSize; z++)
                                {
                                    for (uint32_t y = innerStartPos.y; y < ChunkIterationSize; y++)
                                    {
                                        for (uint32_t x = innerStartPos.x; x < ChunkIterationSize; x++)
                                        {
                                            // Goxel uses a left handed coordinate system, VCore uses a right handed one. So we need to convert the coordinates.
                                            uint32_t p = imgData[(GoxelChunkSize - x - 1) + GoxelChunkSize * z + GoxelChunkSize * GoxelChunkSize * y];
                                            if((p & 0xFF000000) != 0)
                                            {
                                                auto mat = m_Materials[chunkinfo.Material];
                                                mat.Transparency = 1.0 - (((p >> 24) & 0xFF) / 0xFF);
            
                                                // Tries to find the material.
                                                uint32_t matIdx = MaterialManager::FindMaterialSlot(mat);
                                                if(matIdx == UCHAR_MAX)
                                                {
                                                    matIdx = MaterialManager::AddMaterial(mat);
                                                    if(matIdx == UCHAR_MAX)
                                                        matIdx = 0;
                                                }
            
                                                auto insertPos = Math::Vec3i(cx + x, cy + y, cz + z);
                                                p_Chunk->insert({insertPos, CVoxel(p, matIdx)});
                                            }
                                        }
                                    }
                                }
            
                                delete[] imgData;
                            }
                        }
                    }
                }
            }
        }
    }
} // namespace VCore
