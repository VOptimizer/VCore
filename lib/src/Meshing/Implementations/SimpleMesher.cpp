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

#include "../../Misc/Helper.hpp"
#include "../FaceMask.hpp"

namespace VCore
{
    struct SFaceInfo
    {
        Math::Vec3f V1, V2, V3, V4, Normal;   
    };

    const static SFaceInfo FACE_INFOS[6] = {
        { { 0, 1, 0 }, { 0, 1, 1 }, { 0, 0, 0 }, { 0, 0, 1 }, Math::Vec3f::LEFT },
        { { 0, 1, 0 }, { 0, 1, 1 }, { 0, 0, 0 }, { 0, 0, 1 }, Math::Vec3f::RIGHT },

        { { 0, 0, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 1, 0, 1 }, Math::Vec3f::DOWN },
        { { 0, 0, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 1, 0, 1 }, Math::Vec3f::UP },

        { { 0, 1, 0 }, { 1, 1, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, Math::Vec3f::BACK },
        { { 0, 1, 0 }, { 1, 1, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, Math::Vec3f::FRONT },
    };

    SMeshChunk CSimpleMesher::GenerateMeshChunk(VoxelModel m, const SChunkMeta &_Chunk, bool Opaque)
    {
        (void)Opaque;

        CMeshBuilder builder(m_SurfaceFactory);
        builder.AddTextures(m->Textures);

        if(m->TexturingType == TexturingTypes::TEXTURED)
            builder.SetTextureMap(&m->TextureMapping);

        const CBBox chunkBBox(_Chunk.TotalBBox.Beg, _Chunk.TotalBBox.GetSize());

        // For all 3 axis (x, y, z)
        for (size_t axis = 0; axis < 3; axis++)
        {
            // This logic calculates the index of one of the three other axis.
            int axis1 = (axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
            int axis2 = (axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y

            CFaceMask mask;
            auto masks = mask.Generate(m, _Chunk, axis);

            for (auto &&depth : masks)
            {
                for (auto &&key : depth.second)
                {
                    auto parts = split(key.first, "_");

                    for (int widthAxis = 0; widthAxis < CHUNK_SIZE; widthAxis++)
                    {
                        auto faces = key.second.Bits[widthAxis];
                        GenerateQuads(builder, faces, depth.first, widthAxis, true, Math::Vec3i(axis, axis1, axis2), _Chunk, m, parts);

                        faces = key.second.Bits[widthAxis + CHUNK_SIZE];
                        GenerateQuads(builder, faces, depth.first + 1, widthAxis, false, Math::Vec3i(axis, axis1, axis2), _Chunk, m, parts);
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

    void CSimpleMesher::GenerateQuads(CMeshBuilder &_Builder, BITMASK_TYPE _Faces, int depth, int width, bool isFront, const Math::Vec3i &_Axis, const SChunkMeta &_Chunk, const VoxelModel &_Model, const std::vector<std::string> &_Parts)
    {
        unsigned heightPos = 0;
        while ((heightPos <= (CHUNK_SIZE + 2)) && (_Faces >> heightPos))
        {
            heightPos += CountTrailingZeroBits(_Faces >> heightPos);
            auto &faceInfo = FACE_INFOS[_Axis.x * 2 + (isFront ? 0 : 1)];

            Material mat;
            if(std::stoi(_Parts[0]) < _Model->Materials.size())
                mat = _Model->Materials[std::stoi(_Parts[0])];

            for (; heightPos <= (CHUNK_SIZE + 2); heightPos++)
            {
                if(((_Faces >> heightPos) & 0x1) == 0)
                    break;

                Math::Vec3i position;
                position.v[_Axis.x] = _Chunk.TotalBBox.Beg.v[_Axis.x] + depth;
                position.v[_Axis.y] = _Chunk.TotalBBox.Beg.v[_Axis.y] + heightPos;
                position.v[_Axis.z] = _Chunk.TotalBBox.Beg.v[_Axis.z] + width;

                _Builder.AddFace((faceInfo.V1 + position), (faceInfo.V2 + position), (faceInfo.V3 + position), (faceInfo.V4 + position), faceInfo.Normal, std::stoi(_Parts[1]), mat);
            }
        }
    }
}
