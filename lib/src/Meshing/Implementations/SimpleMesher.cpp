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

#include "SimpleMesher.hpp"
#include <algorithm>
#include <VoxelOptimizer/Meshing/MeshBuilder.hpp>

namespace VoxelOptimizer
{
    std::list<SMeshChunk> CSimpleMesher::GenerateChunks(VoxelMesh m, bool onlyDirty)
    {
        std::list<SMeshChunk> ret;
        auto &voxels = m->GetVoxels();

        // Mesh all opaque voxels
        m_Voxels = voxels.queryVisible(true);
    	std::list<SChunk> chunks;
    	if(!onlyDirty)
            chunks = voxels.queryBBoxes();
        else
            chunks = voxels.queryDirtyChunks();

        for (auto &&c : chunks)        
        {
            SMeshChunk chunk;
            chunk.UniqueId = c.UniqueId;
            chunk.InnerBBox = c.InnerBBox;
            chunk.TotalBBox = c.TotalBBox;
            chunk.Mesh = GenerateMesh(m, c.InnerBBox);
            ret.push_back(chunk);
        }

        // Mesh all transparent voxels
        m_Voxels = voxels.queryVisible(false);
        if(!m_Voxels.empty())
        {
            for (auto &&c : chunks)        
            {
                auto mesh = GenerateMesh(m, c.InnerBBox);
                auto it = std::find_if(ret.begin(), ret.end(), [&c](const SMeshChunk &_Chunk) {
                    return _Chunk.UniqueId == c.UniqueId;
                });

                // Merge the transparent chunk with the opaque one.
                if(it != ret.end())
                {
                    CMeshBuilder builder;
                    builder.Merge(it->Mesh, std::vector<Mesh>() = { mesh });
                }
                else
                {
                    SMeshChunk chunk;
                    chunk.UniqueId = c.UniqueId;
                    chunk.InnerBBox = c.InnerBBox;
                    chunk.TotalBBox = c.TotalBBox;
                    chunk.Mesh = mesh;
                    ret.push_back(chunk);
                }
            }
        }

        m_Voxels.clear();
        return ret;
    }

    Mesh CSimpleMesher::GenerateMesh(VoxelMesh m, const CBBox &chunk)
    {
        CMeshBuilder builder;
        builder.AddTextures(m->Colorpalettes);

        auto bbox = m->BBox;
        CVector beg = bbox.Beg;
        std::swap(beg.y, beg.z);
        beg.z *= -1;

        CVector boxCenter = bbox.GetSize() / 2;
        std::swap(boxCenter.y, boxCenter.z);
        boxCenter.z *= -1;

        for(int x = chunk.Beg.x; x < chunk.End.x; x++)
        {
            for(int y = chunk.Beg.y; y < chunk.End.y; y++)
            {
                for(int z = chunk.Beg.z; z < chunk.End.z; z++)
                {
                    auto it = m_Voxels.find(CVectori(x, y, z));
                    if(it != m_Voxels.end())
                    {
                        CVectori vpos = it->first;
                        Voxel v = it->second;
                        for (uint8_t i = 0; i < 6; i++)
                        {
                            CVoxel::Visibility visiblity = (CVoxel::Visibility )((uint8_t)v->VisibilityMask & (uint8_t)(1 << i));

                            // Invisible
                            if(visiblity == CVoxel::Visibility::INVISIBLE)
                                continue;

                            CVector v1, v2, v3, v4, Normal;                            
                            switch (visiblity)
                            {
                                case CVoxel::Visibility::UP:
                                case CVoxel::Visibility::DOWN:
                                {
                                    float PosZ = 0;
                                    if(visiblity == CVoxel::Visibility::UP)
                                    {
                                        Normal = CVector(0, 1, 0);
                                        PosZ = 1;
                                    }
                                    else
                                        Normal = CVector(0, -1, 0);


                                    v1 = CVector(vpos.x, vpos.z + PosZ, -vpos.y - 1.f) - boxCenter;
                                    v2 = CVector(vpos.x, vpos.z + PosZ, -vpos.y) - boxCenter;
                                    v3 = CVector(vpos.x + 1.f, vpos.z + PosZ, -vpos.y) - boxCenter;
                                    v4 = CVector(vpos.x + 1.f, vpos.z + PosZ, -vpos.y - 1.f) - boxCenter;
                                }break;

                                case CVoxel::Visibility::LEFT:
                                case CVoxel::Visibility::RIGHT:
                                {
                                    float Posx = 0;
                                    if(visiblity == CVoxel::Visibility::RIGHT)
                                    {
                                        Normal = CVector(1, 0, 0);
                                        Posx = 1;
                                    }
                                    else
                                        Normal = CVector(-1, 0, 0);

                                    v1 = CVector(vpos.x + Posx, vpos.z, -vpos.y) - boxCenter;
                                    v2 = CVector(vpos.x + Posx, vpos.z, -vpos.y - 1.f) - boxCenter;
                                    v3 = CVector(vpos.x + Posx, vpos.z + 1.f, -vpos.y - 1.f) - boxCenter;
                                    v4 = CVector(vpos.x + Posx, vpos.z + 1.f, -vpos.y) - boxCenter;
                                }break;

                                case CVoxel::Visibility::FORWARD:
                                case CVoxel::Visibility::BACKWARD:
                                {
                                    float PosY = 0;
                                    if(visiblity == CVoxel::Visibility::FORWARD)
                                    {
                                        Normal = CVector(0, 0, -1);
                                        PosY = -1;
                                    }
                                    else
                                        Normal = CVector(0, 0, 1);

                                    v4 = CVector(vpos.x, vpos.z + 1.f, -vpos.y + PosY) - boxCenter;
                                    v3 = CVector(vpos.x, vpos.z, -vpos.y + PosY) - boxCenter;
                                    v2 = CVector(vpos.x + 1.f, vpos.z, -vpos.y + PosY) - boxCenter;
                                    v1 = CVector(vpos.x + 1.f, vpos.z + 1.f, -vpos.y + PosY) - boxCenter;
                                }break;
                            }

                            builder.AddFace(v1, v2, v3, v4, Normal, v->Color, m->Materials[v->Material]);
                        }
                    }
                }
            }
        }

        return builder.Build();
    }
} // namespace VoxelOptimizer
