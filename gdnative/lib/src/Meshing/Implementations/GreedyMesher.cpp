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

#include "Slicer/Slicer.hpp"
#include "Slicer/BetterSlicer.hpp"
#include <VoxelOptimizer/Meshing/MeshBuilder.hpp>
#include <VoxelOptimizer/Meshing/VerticesReducer.hpp>
#include <vector>

#include "GreedyMesher.hpp"

namespace VoxelOptimizer
{
    std::map<CVector, Mesh> CGreedyMesher::GenerateMeshes(VoxelMesh m)
    {
        std::map<CVector, Mesh> ret;

        auto &voxels = m->GetVoxels();
        m_Voxels = voxels.queryVisible(true);

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        auto chunks = voxels.queryBBoxes();
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        auto count1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();

        size_t idx = 0;

        begin = std::chrono::steady_clock::now();
        for (auto &&c : chunks)
        {
            ret[c.Beg] = GenerateMesh(m, c, true);
            idx++;
        }

        m_Voxels = voxels.queryVisible(false);
        if(!m_Voxels.empty())
        {
            for (auto &&c : chunks)        
            {
                auto mesh = GenerateMesh(m, c, false);
                auto it = ret.find(c.Beg);

                // Merge the transparent chunk with the opaque one.
                if(it != ret.end())
                {
                    CMeshBuilder builder;
                    builder.Merge(it->second, std::vector<Mesh>() = { mesh });
                }
                else
                    ret[c.Beg] = mesh;
            }
        }
        end = std::chrono::steady_clock::now();
        auto count = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
        m_Voxels.clear();
        
        return ret;
    }

    Mesh CGreedyMesher::GenerateMesh(VoxelMesh m, const CBBox &BBox, bool Opaque)
    {
        CMeshBuilder builder;
        builder.AddTextures(m->Colorpalettes());

        auto &materials = m->Materials();

        auto TotalBBox = m->GetBBox();
        CVector Beg = TotalBBox.Beg;
        std::swap(Beg.y, Beg.z);
        Beg.z *= -1;

        CVector BoxCenter = TotalBBox.GetSize() / 2;
        std::swap(BoxCenter.y, BoxCenter.z);
        BoxCenter.z *= -1;

        CSlicer Slicer(m, m_Voxels, Opaque);

        // For all 3 axis (x, y, z)
        for (size_t Axis = 0; Axis < 3; Axis++)
        {
            Slicer.SetActiveAxis(Axis);

            int Axis1 = (Axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
            int Axis2 = (Axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y
            int x[3] = {0};

            // Iterate over each slice of the 3d model.
            for (x[Axis] = BBox.Beg.v[Axis] -1; x[Axis] < BBox.End.v[Axis];)
            {
                ++x[Axis];

                // Foreach slice go over a 2d plane. 
                for (int HeightAxis = BBox.Beg.v[Axis2]; HeightAxis < BBox.End.v[Axis2]; ++HeightAxis)
                {
                    for (int WidthAxis = BBox.Beg.v[Axis1]; WidthAxis < BBox.End.v[Axis1];)
                    {
                        CVector Pos;
                        Pos.v[Axis] = x[Axis] - 1;
                        Pos.v[Axis1] = WidthAxis;
                        Pos.v[Axis2] = HeightAxis;

                        if(Slicer.IsFace(Pos))
                        {
                            int w, h;
                            CVector Normal = Slicer.Normal();
                            int Material = Slicer.Material();
                            int Color = Slicer.Color();

                            //Claculates the width of the rect.
                            for (w = 1; WidthAxis + w < BBox.End.v[Axis1]; w++) 
                            {
                                CVector WPos;
                                WPos.v[Axis1] = w;

                                bool IsFace = Slicer.IsFace(Pos + WPos);
                                if(IsFace)
                                {
                                    CVector FaceNormal = Slicer.Normal();
                                    IsFace = Normal == FaceNormal && Material == Slicer.Material() && Color == Slicer.Color();
                                }

                                if(!IsFace)
                                    break;
                            }

                            bool done = false;
                            for (h = 1; HeightAxis + h < BBox.End.v[Axis2]; h++)
                            {
                                // Check each block next to this quad
                                for (int k = 0; k < w; ++k)
                                {
                                    CVector QuadPos = Pos;
                                    QuadPos.v[Axis1] += k;
                                    QuadPos.v[Axis2] += h;

                                    bool IsFace = Slicer.IsFace(QuadPos);
                                    if(IsFace)
                                    {
                                        CVector FaceNormal = Slicer.Normal();
                                        IsFace = Normal == FaceNormal && Material == Slicer.Material() && Color == Slicer.Color();
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

                            x[Axis1] = WidthAxis;
                            x[Axis2] = HeightAxis;

                            int du[3] = {0};
                            du[Axis1] = w;

                            int dv[3] = {0};
                            dv[Axis2] = h;

                            int I1, I2, I3, I4;                            

                            CVector v1 = CVector(x[0], x[2], -x[1]) - Beg - BoxCenter;
                            CVector v2 = CVector(x[0] + du[0], x[2] + du[2], -x[1] - du[1]) - Beg - BoxCenter;
                            CVector v3 = CVector(x[0] + du[0] + dv[0], x[2] + du[2] + dv[2], -x[1] - du[1] - dv[1]) - Beg - BoxCenter;
                            CVector v4 = CVector(x[0] + dv[0], x[2] + dv[2], -x[1] - dv[1]) - Beg - BoxCenter;

                            std::swap(Normal.y, Normal.z);
                            if(Normal.z != 0)
                                Normal.z *= -1;

                            builder.AddFace(v1, v2, v3, v4, Normal, Color, materials[Material]);
                            Slicer.AddProcessedQuad(CVector(x[0], x[1], x[2]), CVector(du[0] + dv[0], du[1] + dv[1], du[2] + dv[2]));

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

        return builder.Build();
    }
} // namespace VoxelOptimizer