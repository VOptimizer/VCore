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

#include "TransvoxelMesher.hpp"
#include "CellCache.hpp"
#include "TransvoxelTables.hpp"
#include "VCore/Math/Vector.hpp"
#include "VCore/Meshing/Mesh/Vertex.hpp"
#include "VCore/VConfig.hpp"
#include "VCore/Voxel/Storage/Chunk.hpp"
#include <VCore/Meshing/Mesh/MeshBuilder.hpp>
#include <cstdint>
#include <format>

namespace VCore 
{
    template <typename T> int sgn(T p_val) 
    {
        return (T(0) < p_val) - (p_val < T(0));
    }

    Config::bitmask_t ExtractVoxels(const CChunk **p_LocalGrid, const Math::Vec3i &p_Pos)
    {
        auto pos =  GetChunkpos(p_Pos);
        pos = pos - Math::Vec3i(sgn(pos.x) * (Config::ChunkSize - 1), sgn(pos.y) * (Config::ChunkSize - 1), sgn(pos.z) * (Config::ChunkSize - 1));
        auto chunk = p_LocalGrid[(pos.x + 1) + (pos.y + 1) * 3 + (pos.z + 1) * 3 * 3];

        if(chunk)
        {
            auto row = chunk->Mask.GetRowFaces(Math::Vec3i(0, p_Pos.y, p_Pos.z) & Config::InnerChunkMask, 0);
            auto innerX = p_Pos.x & Config::InnerChunkMask;
            return row >> innerX;
        }

        return 0;
    }

    SMeshChunk CTransvoxelMesher::GenerateMeshChunk(VoxelModel p_Mesh, const SChunkMeta &p_Chunk, bool)
    {
        CCellCache cache;

        CMeshBuilder builder(m_SurfaceFactory);
        const CBBox chunkBBox(p_Chunk.TotalBBox.Beg, p_Chunk.TotalBBox.GetSize());
        builder.SelectSurface(0);

        CChunk *localGrid[3 * 3 * 3];
        for (int z = -1; z <= 1; z++) 
        {
            for (int y = -1; y <= 1; y++)
            {
                for (int x = -1; x <= 1; x++)
                {
                    localGrid[(x + 1) + (y + 1) * 3 + (z + 1) * 3 * 3] = p_Mesh->GetChunk(p_Chunk.TotalBBox.Beg + Math::Vec3i(x * Config::ChunkSize, y * Config::ChunkSize, z * Config::ChunkSize));
                }
            }
        }

        // const float t = 0.3;

        for (int z = -1; z < static_cast<int>(Config::ChunkSize); z++) 
        {
            for (int y = -1; y < static_cast<int>(Config::ChunkSize); y++)
            {
                int x = -1;
                Config::bitmask_t voxelCount = Config::ChunkSize;

                while (x < static_cast<int>(Config::ChunkSize)) 
                {
                    Config::bitmask_t voxels[4] = {};

                    // uint64_t signFlags = 0;
                    for (int dy = 0; dy <= 1; dy++)
                    {
                        for (int dz = 0; dz <= 1; dz++)
                        {
                            voxels[dz + 2 * dy] = ExtractVoxels(const_cast<const CChunk**>(localGrid), Math::Vec3i(x, dy + y, dz + z));
                            if(x == -1)
                                voxels[dz + 2 * dy] |= ExtractVoxels(const_cast<const CChunk**>(localGrid), Math::Vec3i(x + 1, dy + y, dz + z)) << 1;
                            else if(x == static_cast<int>(Config::ChunkSize) - 2)
                                voxels[dz + 2 * dy] |= (ExtractVoxels(const_cast<const CChunk**>(localGrid), Math::Vec3i(x + 2, dy + y, dz + z)) & 0x3) << 2;
                        }
                    }

                    const Math::Vec3i corners[8] = {
                        Math::Vec3i(x, y, z) + Math::Vec3i::ONE,
                        Math::Vec3i(x + 1, y, z) + Math::Vec3i::ONE,
                        Math::Vec3i(x, y, z + 1) + Math::Vec3i::ONE,
                        Math::Vec3i(x + 1, y, z + 1) + Math::Vec3i::ONE,

                        Math::Vec3i(x, y + 1, z) + Math::Vec3i::ONE,
                        Math::Vec3i(x + 1, y + 1, z) + Math::Vec3i::ONE,
                        Math::Vec3i(x, y + 1, z + 1) + Math::Vec3i::ONE,
                        Math::Vec3i(x + 1, y + 1, z + 1) + Math::Vec3i::ONE,
                    };

                    const uint8_t visibilityMask = (x > -1 ? 1 : 0) | ((y > -1 ? 1 : 0) << 1) | ((z > -1 ? 1 : 0) << 2);

                    for (Config::bitmask_t i = 0; i < voxelCount - 1; i++) 
                    {
                        const uint16_t cellIdx = ((voxels[0] >> i) & 0x3) | (((voxels[1] >> i) & 0x3) << 2) | (((voxels[2] >> i) & 0x3) << 4) | (((voxels[3] >> i) & 0x3) << 6);

                        if(cellIdx != 0 && cellIdx != 0xFF)
                        {
                            auto cellClass = regularCellClass[cellIdx];
                            auto cellData = &regularCellData[cellClass];

                            uint32_t idx[3];
                            uint32_t counter = 0;

                            auto end = cellData->vertexIndex + (cellData->GetTriangleCount() * 3);
                            for (auto data = cellData->vertexIndex; data != end; data++)
                            {
                                auto edgeData = regularVertexData[cellIdx][*data];
                                auto v0 = (edgeData & 0xF0) >> 4;
                                auto v1 = edgeData & 0xF;

                                auto p0 = corners[v0];
                                auto p1 = corners[v1];

                                auto d1 = ((cellIdx & (1 << v1)) >> v1) ? -1 : 1;
                                auto d0 = ((cellIdx & (1 << v0)) >> v0) ? -1 : 1;

                                const float t = static_cast<float>(d1) / static_cast<float>(d1 - d0);                                
                                auto q = t * Math::Vec3f(p0 + Math::Vec3i(i, 0, 0)) + (1.f - t) * Math::Vec3f(p1 + Math::Vec3i(i, 0, 0));

                                // Maybe one of the stupiest approaches to this problem or I didn't read
                                // the paper correctly, but since V-Core uses different threads for each chunk
                                // it is not possible to generate one big mesh. This leads to the problem, that
                                // there are overlapping faces. To remove them, I check if a face would be generated
                                // at the lower boundary, and check if there is an adjacenting chunk, then skip this face.
                                if(q.z <= 0.5f)
                                {
                                    if(localGrid[4])
                                        continue;
                                }

                                if(q.y <= 0.5f)
                                {
                                    if(localGrid[10])
                                        continue;
                                }

                                if(q.x <= 0.5f)
                                {
                                    if(localGrid[12])
                                        continue;
                                }

                                idx[counter++] = builder.AddVertex(new SVertex(q + chunkBBox.Beg, Math::Vec3f::ZERO, 0));

                                if(counter % 3 == 0)
                                {
                                    builder.AddFace(idx[0], idx[1], idx[2]);
                                    counter = 0;
                                }
                            }
                        }
                    }

                    x += Config::ChunkSize - 1;
                    voxelCount = 3;
                }
            }
        }

        SMeshChunk chunk;
        chunk.UniqueId = p_Chunk.UniqueId;
        chunk.InnerBBox = p_Chunk.InnerBBox;
        chunk.TotalBBox = p_Chunk.TotalBBox;
        chunk.MeshData = builder.Build();

        return chunk;
    }
}