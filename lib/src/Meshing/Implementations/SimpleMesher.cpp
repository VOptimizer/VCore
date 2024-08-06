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
#include <VCore/Meshing/MeshBuilder.hpp>

namespace VCore
{
    struct SFaceInfo
    {
        Math::Vec3f V1, V2, V3, V4, Normal;   
    };

    const static SFaceInfo FACE_INFOS[6] = {
        { { 0, 1, 0 }, { 1, 1, 0 }, { 0, 1, 1 }, { 1, 1, 1 }, Math::Vec3f::UP },
        { { 0, 0, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 1, 0, 1 }, Math::Vec3f::DOWN },

        { { 0, 1, 0 }, { 0, 1, 1 }, { 0, 0, 0 }, { 0, 0, 1 }, Math::Vec3f::LEFT },
        { { 1, 1, 0 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 0, 1 }, Math::Vec3f::RIGHT },

        { { 0, 1, 1 }, { 1, 1, 1 }, { 0, 0, 1 }, { 1, 0, 1 }, Math::Vec3f::FRONT },
        { { 0, 1, 0 }, { 1, 1, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, Math::Vec3f::BACK },
    };

    SMeshChunk CSimpleMesher::GenerateMeshChunk(VoxelModel m, const SChunkMeta &_Chunk, bool Opaque)
    {
        (void)Opaque;

        CMeshBuilder builder;
        builder.AddTextures(m->Textures);

        if(m->TexturingType == TexturingTypes::TEXTURED)
            builder.SetTextureMap(&m->TextureMapping);

        const CBBox chunkBBox(_Chunk.TotalBBox.Beg, _Chunk.TotalBBox.GetSize());

        // Left Axis
        for(int y = _Chunk.InnerBBox.Beg.y; y <= _Chunk.InnerBBox.End.y; y++)
        {
            for(int z = _Chunk.InnerBBox.Beg.z; z <= _Chunk.InnerBBox.End.z; z++)
            {
                uint32_t voxels = _Chunk.Chunk->m_Mask.GetRowFaces(Math::Vec3i(0, y - _Chunk.TotalBBox.Beg.y, z - _Chunk.TotalBBox.Beg.z), 0);
                uint32_t leftFaces = (voxels & (uint32_t)~(voxels << 1)) >> 1;
                uint32_t rightFaces = ((voxels & (uint32_t)~(voxels >> 1)) >> 1) & FACE_MASK;

                for(int x = _Chunk.InnerBBox.Beg.x; x <= _Chunk.InnerBBox.End.x; x++)
                {
                    int shiftBit = x - _Chunk.TotalBBox.Beg.x;

                    if(leftFaces & (1 << shiftBit) || rightFaces & (1 << shiftBit))
                    {
                        Math::Vec3i vpos(x, y, z);
                        Voxel v = _Chunk.Chunk->findVisible(vpos, chunkBBox);

                        // if(v)
                        {
                            Material mat;
                            if(v->Material < (short)m->Materials.size())
                                mat = m->Materials[v->Material];

                            if(leftFaces & (1 << shiftBit))
                            {
                                auto info = FACE_INFOS[2];
                                builder.AddFace((info.V1 + vpos), (info.V2 + vpos), (info.V3 + vpos), (info.V4 + vpos), info.Normal, v->Color, mat);
                            }
                            if(rightFaces & (1 << shiftBit))
                            {
                                auto info = FACE_INFOS[3];
                                builder.AddFace((info.V1 + vpos), (info.V2 + vpos), (info.V3 + vpos), (info.V4 + vpos), info.Normal, v->Color, mat);
                            }
                        }
                    }
                }
            }
        }

        // Up Axis
        for(int z = _Chunk.InnerBBox.Beg.z; z <= _Chunk.InnerBBox.End.z; z++)
        {
            for(int x = _Chunk.InnerBBox.Beg.x; x <= _Chunk.InnerBBox.End.x; x++)
            {
                uint32_t voxels = _Chunk.Chunk->m_Mask.GetRowFaces(Math::Vec3i(x - _Chunk.TotalBBox.Beg.x, 0, z - _Chunk.TotalBBox.Beg.z), 1);
                uint32_t topFaces = (voxels & (uint32_t)~(voxels >> 1)) >> 1;
                uint32_t bottomFaces = ((voxels & (uint32_t)~(voxels << 1)) >> 1) & FACE_MASK;

                for(int y = _Chunk.InnerBBox.Beg.y; y <= _Chunk.InnerBBox.End.y; y++)
                {
                    int shiftBit = y - _Chunk.TotalBBox.Beg.y;

                    if(topFaces & (1 << shiftBit) || bottomFaces & (1 << shiftBit))
                    {
                        Math::Vec3i vpos(x, y, z);
                        Voxel v = _Chunk.Chunk->findVisible(vpos, chunkBBox);

                        // if(v)
                        {
                            Material mat;
                            if(v->Material < (short)m->Materials.size())
                                mat = m->Materials[v->Material];

                            if(topFaces & (1 << shiftBit))
                            {
                                auto info = FACE_INFOS[0];
                                builder.AddFace((info.V1 + vpos), (info.V2 + vpos), (info.V3 + vpos), (info.V4 + vpos), info.Normal, v->Color, mat);
                            }
                            if(bottomFaces & (1 << shiftBit))
                            {
                                auto info = FACE_INFOS[1];
                                builder.AddFace((info.V1 + vpos), (info.V2 + vpos), (info.V3 + vpos), (info.V4 + vpos), info.Normal, v->Color, mat);
                            }
                        }
                    }
                }
            }
        }

        // Front Axis
        for(int y = _Chunk.InnerBBox.Beg.y; y <= _Chunk.InnerBBox.End.y; y++)
        {
            for(int x = _Chunk.InnerBBox.Beg.x; x <= _Chunk.InnerBBox.End.x; x++)
            {
                uint32_t voxels = _Chunk.Chunk->m_Mask.GetRowFaces(Math::Vec3i(x - _Chunk.TotalBBox.Beg.x, y - _Chunk.TotalBBox.Beg.y, 0), 2);
                uint32_t frontFaces = (voxels & (uint32_t)~(voxels >> 1)) >> 1;
                uint32_t backFaces = ((voxels & (uint32_t)~(voxels << 1)) >> 1) & FACE_MASK;

                for(int z = _Chunk.InnerBBox.Beg.z; z <= _Chunk.InnerBBox.End.z; z++)
                {
                    int shiftBit = z - _Chunk.TotalBBox.Beg.z;

                    if(frontFaces & (1 << shiftBit) || backFaces & (1 << shiftBit))
                    {
                        Math::Vec3i vpos(x, y, z);
                        Voxel v = _Chunk.Chunk->findVisible(vpos, chunkBBox);

                        // if(v)
                        {
                            Material mat;
                            if(v->Material < (short)m->Materials.size())
                                mat = m->Materials[v->Material];

                            if(frontFaces & (1 << shiftBit))
                            {
                                auto info = FACE_INFOS[4];
                                builder.AddFace((info.V1 + vpos), (info.V2 + vpos), (info.V3 + vpos), (info.V4 + vpos), info.Normal, v->Color, mat);
                            }
                            if(backFaces & (1 << shiftBit))
                            {
                                auto info = FACE_INFOS[5];
                                builder.AddFace((info.V1 + vpos), (info.V2 + vpos), (info.V3 + vpos), (info.V4 + vpos), info.Normal, v->Color, mat);
                            }
                        }
                    }
                }
            }
        }

        // for(int x = _Chunk.InnerBBox.Beg.x; x <= _Chunk.InnerBBox.End.x; x++)
        // {
        //     for(int y = _Chunk.InnerBBox.Beg.y; y <= _Chunk.InnerBBox.End.y; y++)
        //     {
        //         for(int z = _Chunk.InnerBBox.Beg.z; z <= _Chunk.InnerBBox.End.z; z++)
        //         {
        //             Math::Vec3i vpos(x, y, z);
        //             Voxel v = _Chunk.Chunk->findVisible(vpos, chunkBBox);
        //             if(v)
        //             {
        //                 for (uint8_t i = 0; i < 6; i++)
        //                 {
        //                     CVoxel::Visibility visiblity = (CVoxel::Visibility )((uint8_t)v->VisibilityMask & (uint8_t)(1 << i));

        //                     // Invisible
        //                     if(visiblity == CVoxel::Visibility::INVISIBLE)
        //                         continue;

        //                     Material mat;
        //                     if(v->Material < (short)m->Materials.size())
        //                         mat = m->Materials[v->Material];

        //                     auto info = FACE_INFOS[i];
        //                     builder.AddFace((info.V1 + vpos), (info.V2 + vpos), (info.V3 + vpos), (info.V4 + vpos), info.Normal, v->Color, mat);
        //                 }
        //             }
        //         }
        //     }
        // }

        SMeshChunk chunk;
        chunk.UniqueId = _Chunk.UniqueId;
        chunk.InnerBBox = _Chunk.InnerBBox;
        chunk.TotalBBox = _Chunk.TotalBBox;
        chunk.MeshData = builder.Build();

        return chunk;
    }
}
