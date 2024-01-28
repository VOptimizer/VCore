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
        for(int x = _Chunk.InnerBBox.Beg.x; x <= _Chunk.InnerBBox.End.x; x++)
        {
            for(int y = _Chunk.InnerBBox.Beg.y; y <= _Chunk.InnerBBox.End.y; y++)
            {
                for(int z = _Chunk.InnerBBox.Beg.z; z <= _Chunk.InnerBBox.End.z; z++)
                {
                    Math::Vec3i vpos(x, y, z);
                    Voxel v = _Chunk.Chunk->findVisible(vpos, chunkBBox);
                    if(v)
                    {
                        for (uint8_t i = 0; i < 6; i++)
                        {
                            CVoxel::Visibility visiblity = (CVoxel::Visibility )((uint8_t)v->VisibilityMask & (uint8_t)(1 << i));

                            // Invisible
                            if(visiblity == CVoxel::Visibility::INVISIBLE)
                                continue;

                            Material mat;
                            if(v->Material < (short)m->Materials.size())
                                mat = m->Materials[v->Material];

                            auto info = FACE_INFOS[i];
                            builder.AddFace((info.V1 + vpos), (info.V2 + vpos), (info.V3 + vpos), (info.V4 + vpos), info.Normal, v->Color, mat);
                        }
                    }
                }
            }
        }

        SMeshChunk chunk;
        chunk.UniqueId = _Chunk.UniqueId;
        chunk.InnerBBox = _Chunk.InnerBBox;
        chunk.TotalBBox = _Chunk.TotalBBox;
        chunk.MeshData = builder.Build();

        return chunk;
    }
}
