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

#include <VoxelOptimizer/Meshers/SimpleMesher.hpp>
#include <algorithm>

namespace VoxelOptimizer
{
    std::map<CVector, Mesh> CSimpleMesher::GenerateMeshes(VoxelMesh m)
    {
        std::map<CVector, Mesh> Ret;
        m_CurrentUsedMaterials = m->Materials();

        auto Chunks = m->GetChunksToRemesh();
        auto BBox = m->GetBBox();
        CVector Beg = BBox.Beg;
        std::swap(Beg.y, Beg.z);
        Beg.z *= -1;

        CVector BoxCenter = BBox.GetSize() / 2;
        std::swap(BoxCenter.y, BoxCenter.z);
        BoxCenter.z *= -1;

        for (auto &&c : Chunks)
        {          
            Mesh M = Mesh(new SMesh());
            M->Textures = m->Colorpalettes();

            for(float x = c->BBox.Beg.x; x < c->BBox.End.x; x++)
            {
                for(float y = c->BBox.Beg.y; y < c->BBox.End.y; y++)
                {
                    for(float z = c->BBox.Beg.z; z < c->BBox.End.z; z++)
                    {
                        Voxel v = m->GetVoxel(CVector(x, y, z));

                        if(v && v->IsVisible())
                        {
                            for (uint8_t i = 0; i < 6; i++)
                            {
                                CVoxel::Visibility visiblity = (CVoxel::Visibility )((uint8_t)v->VisibilityMask & (uint8_t)(1 << i));

                                // Invisible
                                if(visiblity == CVoxel::Visibility::INVISIBLE)
                                    continue;

                                CVector v1, v2, v3, v4, Normal;// = v->Normals[i];
                                // std::swap(Normal.y, Normal.z);
                                
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


                                        v1 = CVector(v->Pos.x, v->Pos.z + PosZ, -v->Pos.y - 1.f) - BoxCenter;
                                        v2 = CVector(v->Pos.x, v->Pos.z + PosZ, -v->Pos.y) - BoxCenter;
                                        v3 = CVector(v->Pos.x + 1.f, v->Pos.z + PosZ, -v->Pos.y) - BoxCenter;
                                        v4 = CVector(v->Pos.x + 1.f, v->Pos.z + PosZ, -v->Pos.y - 1.f) - BoxCenter;
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

                                        v1 = CVector(v->Pos.x + Posx, v->Pos.z, -v->Pos.y) - BoxCenter;
                                        v2 = CVector(v->Pos.x + Posx, v->Pos.z, -v->Pos.y - 1.f) - BoxCenter;
                                        v3 = CVector(v->Pos.x + Posx, v->Pos.z + 1.f, -v->Pos.y - 1.f) - BoxCenter;
                                        v4 = CVector(v->Pos.x + Posx, v->Pos.z + 1.f, -v->Pos.y) - BoxCenter;
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

                                        v4 = CVector(v->Pos.x, v->Pos.z + 1.f, -v->Pos.y + PosY) - BoxCenter;
                                        v3 = CVector(v->Pos.x, v->Pos.z, -v->Pos.y + PosY) - BoxCenter;
                                        v2 = CVector(v->Pos.x + 1.f, v->Pos.z, -v->Pos.y + PosY) - BoxCenter;
                                        v1 = CVector(v->Pos.x + 1.f, v->Pos.z + 1.f, -v->Pos.y + PosY) - BoxCenter;
                                     }break;
                                }

                                AddFace(M, v1 - Beg, v2 - Beg, v3 - Beg, v4 - Beg, Normal, v->Color, v->Material);
                            }
                        }
                    }
                }
            }

            M->ModelMatrix = CalculateModelMatrix(m->GetSceneNode());
            Ret[c->BBox.Beg] = M;
            ClearCache();
        }

        return Ret;
    }
} // namespace VoxelOptimizer
