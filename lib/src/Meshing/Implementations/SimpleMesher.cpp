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
#include <VCore/Meshing/MaterialManager.hpp>

namespace VCore
{
    struct SAmbientOcclusionDirection
    {
        Math::Vec3i Side1; // Up
        Math::Vec3i Side2; // Right
        Math::Vec3i Corner; // Up - Right
    };

    struct SFaceInfo
    {
        Math::Vec3f V1, V2, V3, V4, Normal;
        SAmbientOcclusionDirection AmbientOcclusionDirections[4];
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
        { { 0, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 }, { 0, 1, 1 }, Math::Vec3f::LEFT,
            { 
                { 
                    { -1, -1, 0 }, { -1, 0, -1 }, { -1, -1, -1 }
                },
                { 
                    { -1, -1, 0 }, { -1, 0, 1 }, { -1, -1, 1 }
                },
                { 
                    { -1, 1, 0 }, { -1, 0, -1 }, { -1, 1, -1 }
                },
                { 
                    { -1, 1, 0 }, { -1, 0, 1 }, { -1, 1, 1 }
                }
            } 
        },
        { { 0, 0, 1 }, { 0, 0, 0 }, { 0, 1, 1 }, { 0, 1, 0 }, Math::Vec3f::RIGHT,  
            { 
                { 
                    { 0, -1, 0 }, { 0, 0, 1 }, { 0, -1, 1 }
                },
                { 
                    { 0, -1, 0 }, { 0, 0, -1 }, { 0, -1, -1 }
                },
                { 
                    { 0, 1, 0 }, { 0, 0, 1 }, { 0, 1, 1 }
                },
                { 
                    { 0, 1, 0 }, { 0, 0, -1 }, { 0, 1, -1 }
                }
            } 
        },

        { { 0, 0, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 1, 0, 1 }, Math::Vec3f::DOWN,
            { 
                { 
                    { 0, -1, -1 }, { -1, -1, 0 }, { -1, -1, -1 }
                },
                { 
                    { 0, -1, -1 }, { 1, -1, 0 }, { 1, -1, -1 }
                },
                { 
                    { 0, -1, 1 }, { -1, -1, 0 }, { -1, -1, 1 }
                },
                { 
                    { 0, -1, 1 }, { 1, -1, 0 }, { 1, -1, 1 }
                }
            } 
        },
        { { 1, 0, 0 }, { 0, 0, 0 }, { 1, 0, 1 }, { 0, 0, 1 }, Math::Vec3f::UP, 
            { 
                { 
                    { 0, 0, -1 }, { 1, 0, 0 }, { 1, 0, -1 }
                },
                { 
                    { 0, 0, -1 }, { -1, 0, 0 }, { -1, 0, -1 }
                },
                { 
                    { 0, 0, 1 }, { 1, 0, 0 }, { 1, 0, 1 }
                },
                { 
                    { 0, 0, 1 }, { -1, 0, 0 }, { -1, 0, 1 }
                } 
            }
        },

        { { 0, 0, 0 }, { 0, 1, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, Math::Vec3f::BACK,
            { 
                { 
                    { 0, -1, -1 }, { -1, 0, -1 }, { -1, -1, -1 }
                },
                { 
                    { 0, 1, -1 }, { -1, 0, -1 }, { -1, 1, -1 }
                },
                { 
                    { 0, -1, -1 }, { 1, 0, -1 }, { 1, -1, -1 }
                },
                { 
                    { 0, 1, -1 }, { 1, 0, -1 }, { 1, 1, -1 }
                }
            }
        },
        { { 0, 1, 0 }, { 0, 0, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, Math::Vec3f::FRONT,
            { 
                { 
                    { 0, 1, 0 }, { -1, 0, 0 }, { -1, 1, 0 }
                },
                { 
                    { 0, -1, 0 }, { -1, 0, 0 }, { -1, -1, 0 }
                },
                { 
                    { 0, 1, 0 }, { 1, 0, 0 }, { 1, 1, 0 }
                },
                { 
                    { 0, -1, 0 }, { 1, 0, 0 }, { 1, -1, 0 }
                }
            }
        },
    };

    SMeshChunk CSimpleMesher::GenerateMeshChunk(VoxelModel p_Mesh, const SChunkMeta &p_Chunk, bool)
    {
        CMeshBuilder builder(m_SurfaceFactory);

        const CBBox chunkBBox(p_Chunk.TotalBBox.Beg, p_Chunk.TotalBBox.GetSize());

        // For all 3 axis (x, y, z)
        for (size_t axis = 0; axis < 3; axis++)
        {
            // This logic calculates the index of one of the three other axis.
            int axis1 = (axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
            int axis2 = (axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y

            CFaceMask mask;
            auto masks = mask.Generate(p_Mesh, p_Chunk, axis);

            for (auto &&depth : masks)
            {
                for (auto &&key : depth.second)
                {
                    // auto parts = split(key.first, "_");
                    auto voxel = *(CVoxel*)&key.first;

                    builder.SelectSurface(voxel.GetMaterial());

                    // Column connections
                    IndexPair indexFrontCache[Config::ChunkSize] = {};
                    IndexPair indexBackCache[Config::ChunkSize] = {};

                    for (uint32_t widthAxis = 0; widthAxis < Config::ChunkSize; widthAxis++)
                    {
                        auto faces = key.second.Bits[widthAxis];
                        GenerateQuads(builder, faces, depth.first, widthAxis, true, Math::Vec3i(axis, axis1, axis2), p_Chunk, p_Mesh, voxel, indexFrontCache);

                        faces = key.second.Bits[widthAxis + Config::ChunkSize];
                        GenerateQuads(builder, faces, depth.first + 1, widthAxis, false, Math::Vec3i(axis, axis1, axis2), p_Chunk, p_Mesh, voxel, indexBackCache);
                    }
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

    inline uint8_t GenerateAO(uint8_t p_Side1, uint8_t p_Side2, uint8_t p_Corner)
    {
        if(p_Side1 && p_Side2)
            return 0;

        return 3 - (p_Side1 + p_Side2 + p_Corner);
    }

    inline uint32_t AddVertex(CMeshBuilder &p_Builder, const VoxelModel &_Model, const Math::Vec3i p_Position, const SAmbientOcclusionDirection &p_Direction, const Math::Vec3f &p_Vertex, const Math::Vec3f &p_Normal, const uint32_t p_Color, uint8_t &p_Ao)
    {
        auto vertex = p_Vertex + p_Position;

        // TODO: Makes everything 30ms slower (at my machine)
        // auto side1 = _Model->find(_Direction.Side1 + _Position);
        // auto side2 = _Model->find(_Direction.Side2 + _Position);
        // auto corner = _Model->find(_Direction.Corner + _Position);

        // _Ao = GenerateAO(side1 != _Model->end(), side2 != _Model->end(), corner != _Model->end());
        return p_Builder.AddVertex(new SVertex(vertex, p_Normal, p_Color, p_Ao));
    }

    void CSimpleMesher::GenerateQuads(CMeshBuilder &p_Builder, Config::bitmask_t p_Faces, int p_Depth, int p_Width, bool p_isFront, const Math::Vec3i &p_Axis, const SChunkMeta &p_Chunk, const VoxelModel &p_Model, const CVoxel& p_Voxel, IndexPair *p_Cache)
    {
        // Last two indices of the last quad.
        uint32_t lastLeftIdx = 0, lastRightIdx = 0;
        uint8_t lastLeftAO = 0, lastRightAO = 0;
        IndexPair localCache[Config::ChunkSize] = {};
        auto color = p_Voxel.GetColor();

        // Math::Vec2f uv;
        // auto textures = _Builder.GetTextures();
        // if(textures && !textures->empty())
        //     uv = Math::Vec2f(((float)(_Voxel.Color + 0.5f)) / textures->at(TextureType::DIFFIUSE)->GetSize().x, 0.5f);

        Config::bitmask_t heightPos = 0;
        while ((heightPos <= Config::ChunkSize) && (p_Faces >> heightPos))
        {
            auto zeros = heightPos >= Config::ChunkSize ? 0 : CountTrailingZeroBits(p_Faces >> heightPos);

            // Are there any gaps between the quads? If so reset the last save indices
            // Bits zero count > 0
            if(zeros)
                lastLeftIdx = lastRightIdx = 0;

            heightPos += zeros;
            if(heightPos >= Config::ChunkSize)
                break;

            auto &faceInfo = FACE_INFOS[p_Axis.x * 2 + (p_isFront ? 0 : 1)];
            for (; heightPos < Config::ChunkSize; heightPos++)
            {
                if(((p_Faces >> heightPos) & 0x1) == 0)
                    break;

                Math::Vec3i position;
                position.v[p_Axis.x] = p_Chunk.TotalBBox.Beg.v[p_Axis.x] + p_Depth;
                position.v[p_Axis.y] = p_Chunk.TotalBBox.Beg.v[p_Axis.y] + heightPos;
                position.v[p_Axis.z] = p_Chunk.TotalBBox.Beg.v[p_Axis.z] + p_Width;

                // Make room for the new quad.
                // Bits zero count == 0
                uint32_t idx1 = lastLeftIdx, idx2 = lastRightIdx, idx3 = 0, idx4 = 0;

                uint8_t ao1 = lastLeftAO, ao2 = lastRightAO, ao3 = 0, ao4 = 0;

                // Did we have vertices of the last quad? If not, create two new ones.
                // Bits zero count > 0
                if(!lastLeftIdx)
                {
                    if(heightPos < Config::ChunkSize)
                    {
                        if(p_isFront)
                        {
                            if(p_Cache[heightPos].Instantiated)
                            {
                                idx1 = p_Cache[heightPos].Idx2;
                                ao1 = p_Cache[heightPos].AO2;
                            }
                            else if(heightPos && p_Cache[heightPos - 1].Instantiated)
                            {
                                idx1 = p_Cache[heightPos].Idx4;
                                ao1 = p_Cache[heightPos].AO4;
                            }
                        }
                        else
                        {
                            if(p_Cache[heightPos].Instantiated)
                            {
                                idx2 = p_Cache[heightPos].Idx2;
                                ao2 = p_Cache[heightPos].AO2;
                            }
                            else if(heightPos && p_Cache[heightPos - 1].Instantiated)
                            {
                                idx2 = p_Cache[heightPos].Idx4;
                                ao2 = p_Cache[heightPos].AO4;
                            }
                        }
                    }

                    if(!idx1)
                        idx1 = AddVertex(p_Builder, p_Model, position, faceInfo.AmbientOcclusionDirections[0], faceInfo.V1, faceInfo.Normal, color, ao1);

                    if(!idx2)
                        idx2 = AddVertex(p_Builder, p_Model, position, faceInfo.AmbientOcclusionDirections[1], faceInfo.V2, faceInfo.Normal, color, ao2);
                }

                if(heightPos < Config::ChunkSize)
                {
                    if(p_isFront)
                    {
                        if(p_Cache[heightPos].Instantiated)
                        {
                            idx3 = p_Cache[heightPos].Idx4;
                            ao3 = p_Cache[heightPos].AO4;
                        }
                        else if((heightPos + 1 < Config::ChunkSize) && p_Cache[heightPos + 1].Instantiated)
                        {
                            idx3 = p_Cache[heightPos].Idx2;
                            ao3 = p_Cache[heightPos].AO2;
                        }
                    }
                    else
                    {
                        if(p_Cache[heightPos].Instantiated)
                        {
                            idx4 = p_Cache[heightPos].Idx4;
                            ao4 = p_Cache[heightPos].AO4;
                        }
                        else if((heightPos + 1 < Config::ChunkSize) && p_Cache[heightPos + 1].Instantiated)
                        {
                            idx4 = p_Cache[heightPos].Idx2;
                            ao4 = p_Cache[heightPos].AO2;
                        }
                    }
                }

                if(!idx3)
                    idx3 = AddVertex(p_Builder, p_Model, position, faceInfo.AmbientOcclusionDirections[2], faceInfo.V3, faceInfo.Normal, color, ao3);

                if(!idx4)
                    idx4 = AddVertex(p_Builder, p_Model, position, faceInfo.AmbientOcclusionDirections[3], faceInfo.V4, faceInfo.Normal, color, ao4);

                // Save the last two indices.
                lastLeftIdx = idx3;
                lastRightIdx = idx4;

                lastLeftAO = ao3;
                lastRightAO = ao4;

                if(heightPos < Config::ChunkSize)
                {
                    if(p_isFront)
                        localCache[heightPos] = IndexPair(idx2, idx4, ao2, ao4);
                    else
                        localCache[heightPos] = IndexPair(idx1, idx3, ao1, ao3);
                }

                if(ao1 + ao4 > ao2 + ao3)
                {
                    std::swap(idx1, idx2);
                    std::swap(idx2, idx4);
                    std::swap(idx3, idx4);

        // m_CurrentSurface->AddFace(_Idx1, _Idx2, _Idx3);
        // m_CurrentSurface->AddFace(_Idx2, _Idx4, _Idx3);

                    // Save the last two indices.
                    // lastLeftIdx = idx2;
                    // lastRightIdx = idx4;
                }

                // Create the quad.
                p_Builder.AddFace(idx1, idx2, idx3, idx4);
            }
        }

        memcpy(p_Cache, localCache, sizeof(IndexPair) * Config::ChunkSize);
    }
}
