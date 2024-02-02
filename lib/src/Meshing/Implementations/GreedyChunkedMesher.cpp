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

#include <chrono>
#include <future>

#include "Slicer/Slicer.hpp"
#include <VCore/Meshing/MeshBuilder.hpp>
#include <vector>

#include "GreedyChunkedMesher.hpp"

namespace VCore
{
    SMeshChunk CGreedyChunkedMesher::GenerateMeshChunk(VoxelModel m, const SChunkMeta &_Chunk, bool Opaque)
    {
        CMeshBuilder builder;
        builder.AddTextures(m->Textures);

        CBBox BBox = _Chunk.InnerBBox;

        auto &materials = m->Materials;

        CSlicer Slicer(m, Opaque, _Chunk.Chunk, _Chunk.TotalBBox);

        // For all 3 axis (x, y, z)
        for (size_t Axis = 0; Axis < 3; Axis++)
        {
            Slicer.SetActiveAxis(Axis);

            int Axis1 = (Axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
            int Axis2 = (Axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y
            int x[3] = {};

            // Iterate over each slice of the 3d model.
            for (x[Axis] = BBox.Beg.v[Axis] -1; x[Axis] <= BBox.End.v[Axis];)
            {
                ++x[Axis];

                // Foreach slice go over a 2d plane. 
                for (int HeightAxis = BBox.Beg.v[Axis1]; HeightAxis <= BBox.End.v[Axis1]; ++HeightAxis)
                {
                    for (int WidthAxis = BBox.Beg.v[Axis2]; WidthAxis <= BBox.End.v[Axis2];)
                    {
                        Math::Vec3i Pos;
                        Pos.v[Axis] = x[Axis] - 1;
                        Pos.v[Axis1] = HeightAxis;
                        Pos.v[Axis2] = WidthAxis;

                        if(Slicer.IsFace(Pos))
                        {
                            int w, h;
                            Math::Vec3i Normal = Slicer.Normal();
                            int material = Slicer.Material();
                            int Color = Slicer.Color();

                            //Claculates the width of the rect.
                            for (w = 1; WidthAxis + w <= BBox.End.v[Axis2]; w++) 
                            {
                                Math::Vec3i WPos;
                                WPos.v[Axis2] = w;

                                bool IsFace = Slicer.IsFace(Pos + WPos);
                                if(IsFace)
                                {
                                    Math::Vec3i FaceNormal = Slicer.Normal();
                                    IsFace = Normal == FaceNormal && material == Slicer.Material() && Color == Slicer.Color();
                                }

                                if(!IsFace)
                                    break;
                            }

                            bool done = false;
                            for (h = 1; HeightAxis + h <= BBox.End.v[Axis1]; h++)
                            {
                                // Check each block next to this quad
                                for (int k = 0; k < w; ++k)
                                {
                                    Math::Vec3i QuadPos = Pos;
                                    QuadPos.v[Axis2] += k;
                                    QuadPos.v[Axis1] += h;

                                    bool IsFace = Slicer.IsFace(QuadPos);
                                    if(IsFace)
                                    {
                                        Math::Vec3i FaceNormal = Slicer.Normal();
                                        IsFace = Normal == FaceNormal && material == Slicer.Material() && Color == Slicer.Color();
                                    }

                                    // If there's a hole in the mask, exit
                                    if (!IsFace)
                                    {
                                        done = true;
                                        break;
                                    }
                                }

                                if (done)
                                    break;
                            }

                            x[Axis1] = HeightAxis;
                            x[Axis2] = WidthAxis;

                            int du[3] = {};
                            du[Axis2] = w;

                            int dv[3] = {};
                            dv[Axis1] = h;
                    
                            Math::Vec3f v1 = Math::Vec3f(x[0], x[1], x[2]);
                            Math::Vec3f v2 = Math::Vec3f(x[0] + du[0], x[1] + du[1], x[2] + du[2]);
                            Math::Vec3f v3 = Math::Vec3f(x[0] + dv[0], x[1] + dv[1], x[2] + dv[2]);
                            Math::Vec3f v4 = Math::Vec3f(x[0] + du[0] + dv[0], x[1] + du[1] + dv[1], x[2] + du[2] + dv[2]);

                            // std::swap(Normal.y, Normal.z);
                            // if(Normal.y != 0)
                            //     Normal.y *= -1;

                            Material mat;
                            if(material < (int)materials.size())
                                mat = materials[material];

                            builder.AddFace(v1, v2, v3, v4, Normal, Color, mat);
                            Slicer.AddProcessedQuad(Math::Vec3i(x[0], x[1], x[2]), Math::Vec3i(du[0] + dv[0], du[1] + dv[1], du[2] + dv[2]));

                            // Increment counters and continue
                            WidthAxis += w;
                        }
                        else
                            WidthAxis++;
                    }
                }

                Slicer.ClearQuads();
            }

            // break;
        }

        SMeshChunk chunk;
        chunk.UniqueId = _Chunk.UniqueId;
        chunk.InnerBBox = _Chunk.InnerBBox;
        chunk.TotalBBox = _Chunk.TotalBBox;
        chunk.MeshData = builder.Build();

        return chunk;
    }
}
