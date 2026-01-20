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
#include "VCore/Meshing/Color.hpp"
#include "VCore/VConfig.hpp"
#include <cstdlib>
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
        auto innerStartPos = p_Position & (GoxelChunkSize - 1);

        // TODO: 8 Chunk Path
        for (int cx = p_Position.x; cx < static_cast<int>(p_Position.x + Config::ChunkSize); cx += GoxelChunkSize)
        {
            for (int cy = p_Position.y; cy < static_cast<int>(p_Position.y + Config::ChunkSize); cy += GoxelChunkSize)
            {
                for (int cz = p_Position.z; cz < static_cast<int>(p_Position.z + Config::ChunkSize); cz += GoxelChunkSize)
                {
                    // Goxel chunk position. This is different from VCores chunk position. 
                    // Since Goxel uses a left handed coordinate system, VCore uses a right handed one.
                    auto key = Math::Vec3i(cx, cy, cz);
                    auto it = m_Chunks.find(key);
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
                                            uint32_t p = imgData[x + GoxelChunkSize * z + GoxelChunkSize * GoxelChunkSize * y];
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
            
                                                // Goxel uses a left handed coordinate system, VCore uses a right handed one. So we need to convert the coordinates.
                                                auto insertPos = Math::Vec3i((Config::ChunkSize - 1) - ((cx - p_Position.x) + x), (cy - p_Position.y) + y, (cz - p_Position.z) + z);
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
