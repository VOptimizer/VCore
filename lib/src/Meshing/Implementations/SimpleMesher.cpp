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
    SMeshChunk CSimpleMesher::GenerateMeshChunk(VoxelMesh m, const SChunkMeta &_Chunk, bool Opaque)
    {
        CMeshBuilder builder;
        builder.AddTextures(m->Colorpalettes);

        auto bbox = m->BBox;
        Math::Vec3f beg = bbox.Beg;
        std::swap(beg.y, beg.z);
        beg.z *= -1;

        Math::Vec3f boxCenter = bbox.GetSize() / 2;
        std::swap(boxCenter.y, boxCenter.z);
        boxCenter.z *= -1;

        const CBBox chunkBBox(_Chunk.TotalBBox.Beg, _Chunk.TotalBBox.GetSize());

        for(int x = _Chunk.InnerBBox.Beg.x; x <= _Chunk.InnerBBox.End.x; x++)
        {
            for(int y = _Chunk.InnerBBox.Beg.y; y <= _Chunk.InnerBBox.End.y; y++)
            {
                for(int z = _Chunk.InnerBBox.Beg.z; z <= _Chunk.InnerBBox.End.z; z++)
                {
                    Math::Vec3i vpos(x, y, z);
                    Voxel v = _Chunk.Chunk->findVisible(vpos, chunkBBox, Opaque);
                    if(v)
                    {
                        for (uint8_t i = 0; i < 6; i++)
                        {
                            CVoxel::Visibility visiblity = (CVoxel::Visibility )((uint8_t)v->VisibilityMask & (uint8_t)(1 << i));

                            // Invisible
                            if(visiblity == CVoxel::Visibility::INVISIBLE)
                                continue;

                            Math::Vec3f v1, v2, v3, v4, Normal;                            
                            switch (visiblity)
                            {
                                case CVoxel::Visibility::UP:
                                case CVoxel::Visibility::DOWN:
                                {
                                    float PosZ = 0;
                                    if(visiblity == CVoxel::Visibility::UP)
                                    {
                                        Normal = Math::Vec3f(0, 1, 0);
                                        PosZ = 1;
                                    }
                                    else
                                        Normal = Math::Vec3f(0, -1, 0);


                                    v1 = Math::Vec3f(vpos.x, vpos.z + PosZ, -vpos.y - 1.f) - boxCenter;
                                    v2 = Math::Vec3f(vpos.x, vpos.z + PosZ, -vpos.y) - boxCenter;
                                    v3 = Math::Vec3f(vpos.x + 1.f, vpos.z + PosZ, -vpos.y) - boxCenter;
                                    v4 = Math::Vec3f(vpos.x + 1.f, vpos.z + PosZ, -vpos.y - 1.f) - boxCenter;
                                }break;

                                case CVoxel::Visibility::LEFT:
                                case CVoxel::Visibility::RIGHT:
                                {
                                    float Posx = 0;
                                    if(visiblity == CVoxel::Visibility::RIGHT)
                                    {
                                        Normal = Math::Vec3f(1, 0, 0);
                                        Posx = 1;
                                    }
                                    else
                                        Normal = Math::Vec3f(-1, 0, 0);

                                    v1 = Math::Vec3f(vpos.x + Posx, vpos.z, -vpos.y) - boxCenter;
                                    v2 = Math::Vec3f(vpos.x + Posx, vpos.z, -vpos.y - 1.f) - boxCenter;
                                    v3 = Math::Vec3f(vpos.x + Posx, vpos.z + 1.f, -vpos.y - 1.f) - boxCenter;
                                    v4 = Math::Vec3f(vpos.x + Posx, vpos.z + 1.f, -vpos.y) - boxCenter;
                                }break;

                                case CVoxel::Visibility::FORWARD:
                                case CVoxel::Visibility::BACKWARD:
                                {
                                    float PosY = 0;
                                    if(visiblity == CVoxel::Visibility::FORWARD)
                                    {
                                        Normal = Math::Vec3f(0, 0, -1);
                                        PosY = -1;
                                    }
                                    else
                                        Normal = Math::Vec3f(0, 0, 1);

                                    v4 = Math::Vec3f(vpos.x, vpos.z + 1.f, -vpos.y + PosY) - boxCenter;
                                    v3 = Math::Vec3f(vpos.x, vpos.z, -vpos.y + PosY) - boxCenter;
                                    v2 = Math::Vec3f(vpos.x + 1.f, vpos.z, -vpos.y + PosY) - boxCenter;
                                    v1 = Math::Vec3f(vpos.x + 1.f, vpos.z + 1.f, -vpos.y + PosY) - boxCenter;
                                }break;
                            }

                            builder.AddFace(v1, v2, v3, v4, Normal, v->Color, m->Materials[v->Material]);
                        }
                    }
                }
            }
        }

        SMeshChunk chunk;
        chunk.UniqueId = _Chunk.UniqueId;
        chunk.InnerBBox = _Chunk.InnerBBox;
        chunk.TotalBBox = _Chunk.TotalBBox;
        chunk.Mesh = builder.Build();

        return chunk;
    }
}
