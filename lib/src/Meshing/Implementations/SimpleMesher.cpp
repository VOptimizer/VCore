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
    
    /**
     * The order and the "shape" how the vertices are ordered is important for the index algorithm to function.
     * 
     * This ist the shape we want.
     * 3---4
     *  \
     *   \
     * 1---2
     * 
     * Index 1 or 2 can either be completely new or the last two vertices (3, 4) from the quad before the current one.
     * 
     * Basic Algorithm:
     * 
     * - Count the amount of 0 bits
     *  Zero bits count > 0: Generate new vertices for 1 + 2
     *  Zero bits count == 0: Use the indices of the last 3 + 4
     * - Save new indices for 3 + 4
     * - Repeat
     * 
     * To get the connection to the last column, all indices on that border are saved into an array.
     * 
     * Look at CSimpleMesher::GenerateQuads for a maybe better explaination.
     */
    const static SFaceInfo FACE_INFOS[6] = {
        { { 0, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 }, { 0, 1, 1 }, Math::Vec3f::LEFT },
        { { 0, 0, 1 }, { 0, 0, 0 }, { 0, 1, 1 }, { 0, 1, 0 }, Math::Vec3f::RIGHT },

        { { 0, 0, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 1, 0, 1 }, Math::Vec3f::DOWN },
        { { 1, 0, 0 }, { 0, 0, 0 }, { 1, 0, 1 }, { 0, 0, 1 }, Math::Vec3f::UP },

        { { 0, 0, 0 }, { 0, 1, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, Math::Vec3f::BACK },
        { { 0, 1, 0 }, { 0, 0, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, Math::Vec3f::FRONT },
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
                    // auto parts = split(key.first, "_");
                    auto voxel = (Voxel)&key.first;

                    builder.SelectSurface(m->Materials[voxel->Material]);

                    // Column connections
                    IndexPair indexFrontCache[CHUNK_SIZE] = {};
                    IndexPair indexBackCache[CHUNK_SIZE] = {};

                    for (int widthAxis = 0; widthAxis < CHUNK_SIZE; widthAxis++)
                    {
                        auto faces = key.second.Bits[widthAxis];
                        GenerateQuads(builder, faces, depth.first, widthAxis, true, Math::Vec3i(axis, axis1, axis2), _Chunk, m, voxel, indexFrontCache);

                        faces = key.second.Bits[widthAxis + CHUNK_SIZE];
                        GenerateQuads(builder, faces, depth.first + 1, widthAxis, false, Math::Vec3i(axis, axis1, axis2), _Chunk, m, voxel, indexBackCache);
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

    void CSimpleMesher::GenerateQuads(CMeshBuilder &_Builder, BITMASK_TYPE _Faces, int depth, int width, bool isFront, const Math::Vec3i &_Axis, const SChunkMeta &_Chunk, const VoxelModel &_Model, const Voxel _Voxel, IndexPair *_Cache)
    {
        // Last two indices of the last quad.
        uint32_t lastLeftIdx = 0, lastRightIdx = 0;
        IndexPair localCache[CHUNK_SIZE] = {};

        Math::Vec2f uv;
        auto textures = _Builder.GetTextures();
        if(textures && !textures->empty())
            uv = Math::Vec2f(((float)(_Voxel->Color + 0.5f)) / textures->at(TextureType::DIFFIUSE)->GetSize().x, 0.5f);

        BITMASK_TYPE heightPos = 0;
        while ((heightPos <= (CHUNK_SIZE + 2)) && (_Faces >> heightPos))
        {
            auto zeros = CountTrailingZeroBits(_Faces >> heightPos);

            // Are there any gaps between the quads? If so reset the last save indices
            // Bits zero count > 0
            if(zeros)
                lastLeftIdx = lastRightIdx = 0;

            heightPos += zeros;
            if(heightPos >= CHUNK_SIZE)
                break;

            auto &faceInfo = FACE_INFOS[_Axis.x * 2 + (isFront ? 0 : 1)];
            for (; heightPos <= (CHUNK_SIZE + 2); heightPos++)
            {
                if(((_Faces >> heightPos) & 0x1) == 0)
                    break;

                Math::Vec3i position;
                position.v[_Axis.x] = _Chunk.TotalBBox.Beg.v[_Axis.x] + depth;
                position.v[_Axis.y] = _Chunk.TotalBBox.Beg.v[_Axis.y] + heightPos;
                position.v[_Axis.z] = _Chunk.TotalBBox.Beg.v[_Axis.z] + width;

                // Make room for the new quad.
                // Bits zero count == 0
                uint32_t idx1 = lastLeftIdx, idx2 = lastRightIdx, idx3 = 0, idx4 = 0;

                // Did we have vertices of the last quad? If not, create two new ones.
                // Bits zero count > 0
                if(!lastLeftIdx)
                {
                    if(heightPos < CHUNK_SIZE)
                    {
                        if(isFront)
                        {
                            if(_Cache[heightPos].Instantiated)
                                idx1 = _Cache[heightPos].Idx2;
                            else if(heightPos && _Cache[heightPos - 1].Instantiated)
                                idx1 = _Cache[heightPos].Idx4;
                        }
                        else
                        {
                            if(_Cache[heightPos].Instantiated)
                                idx2 = _Cache[heightPos].Idx2;
                            else if(heightPos && _Cache[heightPos - 1].Instantiated)
                                idx2 = _Cache[heightPos].Idx4;
                        }
                    }

                    if(!idx1)
                        idx1 = _Builder.AddVertex(SVertex(faceInfo.V1 + position, faceInfo.Normal, uv));

                    if(!idx2)
                        idx2 = _Builder.AddVertex(SVertex(faceInfo.V2 + position, faceInfo.Normal, uv));
                }

                if(heightPos < CHUNK_SIZE)
                {
                    if(isFront)
                    {
                        if(_Cache[heightPos].Instantiated)
                            idx3 = _Cache[heightPos].Idx4;
                        else if((heightPos + 1 < CHUNK_SIZE) && _Cache[heightPos + 1].Instantiated)
                            idx3 = _Cache[heightPos].Idx2;
                    }
                    else
                    {
                        if(_Cache[heightPos].Instantiated)
                            idx4 = _Cache[heightPos].Idx4;
                        else if((heightPos + 1 < CHUNK_SIZE) && _Cache[heightPos + 1].Instantiated)
                            idx4 = _Cache[heightPos].Idx2;
                    }
                }

                if(!idx3)
                    idx3 = _Builder.AddVertex(SVertex(faceInfo.V3 + position, faceInfo.Normal, uv));

                if(!idx4)
                    idx4 = _Builder.AddVertex(SVertex(faceInfo.V4 + position, faceInfo.Normal, uv));

                // Save the last two indices.
                lastLeftIdx = idx3;
                lastRightIdx = idx4;

                if(heightPos < CHUNK_SIZE)
                {
                    if(isFront)
                        localCache[heightPos] = IndexPair(idx2, idx4);
                    else
                        localCache[heightPos] = IndexPair(idx1, idx3);
                }

                // Create the quad.
                _Builder.AddFace(idx1, idx2, idx3, idx4);
            }
        }

        memcpy(_Cache, localCache, sizeof(IndexPair) * CHUNK_SIZE);
    }
}
