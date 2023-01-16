/*
 * MIT License
 *
 * Copyright (c) 2023 Christian Tost
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

#include "FloodMesher.hpp"
#include <VoxelOptimizer/Meshing/MeshBuilder.hpp>
#include "Triangulation/Triangulate.hpp"

namespace VoxelOptimizer
{
    static const std::pair<CVoxel::Visibility, CVector> FACES[] = {
        {CVoxel::Visibility::UP, CVoxel::FACE_UP},
        {CVoxel::Visibility::DOWN, CVoxel::FACE_DOWN},
        {CVoxel::Visibility::FORWARD, CVoxel::FACE_FORWARD},
        {CVoxel::Visibility::BACKWARD, CVoxel::FACE_BACKWARD},
        {CVoxel::Visibility::LEFT, CVoxel::FACE_LEFT},
        {CVoxel::Visibility::RIGHT, CVoxel::FACE_RIGHT}
    };

    CVector ConvertTo2d(const CVector &_Vec, const CVector &_Normal)
    {
        CVector ret;
        int retI = 0;
        for (size_t n = 0; n < sizeof(_Normal.v); n++)
        {
            if(_Normal.v[n] == 0)
                ret.v[retI++] = _Vec.v[n];
        }
        return ret;
    }

    CVector ConvertTo3d(const CVector &_Vec, const CVector &_Normal)
    {
        CVector ret;
        int retI = 0;
        for (size_t n = 0; n < sizeof(_Normal.v); n++)
        {
            if(_Normal.v[n] == 0)
                ret.v[n] = _Vec.v[retI++];
        }
        return ret;
    }

    std::map<CVector, Mesh> CFloodMesher::GenerateMeshes(VoxelMesh m)
    {
        std::map<CVector, Mesh> ret;

        auto &voxels = m->GetVoxels();
        m_Voxels = voxels.queryVisible(true);

        std::queue<CVectori> voxelQueue;
        voxelQueue.push(m_Voxels.begin()->first);
        while (!voxelQueue.empty())
        {
            CVectori pos = voxelQueue.back();
            voxelQueue.pop();
            GroupShapes(pos);
        }
        
        //     *
        //     **
        //    ***
        //     * 

        std::vector<Mesh> meshes;

        auto &materials = m->Materials();
        for(auto shape : m_Shapes)
        {
            CMeshBuilder builder;
            builder.AddTextures(m->Colorpalettes());

            auto polygons = GeneratePolygons(shape);
            CVector uv = CVector(((float)(shape->m_Voxels.begin()->second->Color + 0.5f)) / m->Colorpalettes()[TextureType::DIFFIUSE]->Size().x, 0.5f, 0);

            std::vector<int> indices;
            CTriangulate::Triangulate(polygons.front(), indices);

            for (size_t i = 0; i < indices.size(); i += 3)
            {
                CVector v1 = ConvertTo3d(polygons.front()[indices[i]], shape->Normal);
                CVector v2 = ConvertTo3d(polygons.front()[indices[i + 1]], shape->Normal);
                CVector v3 = ConvertTo3d(polygons.front()[indices[i + 2]], shape->Normal);

                builder.AddFace(
                    {v1, uv, shape->Normal, materials[shape->m_Voxels.begin()->second->Material], shape->m_Voxels.begin()->second->Material},
                    {v2, uv, shape->Normal, materials[shape->m_Voxels.begin()->second->Material], shape->m_Voxels.begin()->second->Material},
                    {v3, uv, shape->Normal, materials[shape->m_Voxels.begin()->second->Material], shape->m_Voxels.begin()->second->Material});
            }

            meshes.push_back( builder.Build());

        }

        CMeshBuilder builder;
        builder.AddTextures(m->Colorpalettes());
        builder.Merge(nullptr, meshes);

        ret[CVector(0, 0, 0)] = builder.Build();

        return ret;
    }

    void CFloodMesher::GroupShapes(const CVectori &_Position)
    {
        std::map<CVector, std::map<int, SFloodShape*>> faces;
        std::queue<CVectori> voxels;

        Voxel first = m_Voxels.at(_Position);
        Voxel current = first;
        CVectori pos = _Position;

        do
        {
            if(!voxels.empty())
            {
                pos = voxels.back();
                voxels.pop();
                current = m_Voxels[pos];
            }

            for (size_t i = 0; i < sizeof(FACES); i++)
            {
                if((current->VisibilityMask & FACES[i].first) == FACES[i].first)
                {
                    int layer = (pos * FACES[i].second).Sum();

                    SFloodShape *shape;
                    auto it = faces.find(FACES[i].second);
                    if(it != faces.end())
                    {
                        auto it2 = it->second.find(layer);
                        if(it2 != it->second.end())
                            shape = it2->second;
                        else
                        {
                            shape = m_ShapesPool.alloc();
                            shape->Normal = FACES[i].second;

                            it->second[layer] = shape;
                        }
                    }
                    else
                    {
                        shape = m_ShapesPool.alloc();
                        shape->Normal = FACES[i].second;

                        faces[FACES[i].second] = {{layer, shape}};
                    }

                    shape->m_Voxels[pos] = current;
                }
                else
                {
                    CVector newPos = pos + FACES[i].second;
                    Voxel other = m_Voxels[newPos];
                    if(other->Material == first->Material && other->Color == first->Color && other->Transparent == first->Transparent)
                        voxels.push(newPos);
                    else
                        m_Colors.push(newPos);
                }
            }
        } while (!voxels.empty());
    }

    std::vector<std::vector<CVector>> CFloodMesher::GeneratePolygons(SFloodShape *_Shape)
    {
        const static std::map<CVector, CVector> SHIFT_MAP = {
            {CVoxel::FACE_UP, CVoxel::FACE_LEFT},
            {CVoxel::FACE_DOWN, CVoxel::FACE_LEFT},
            {CVoxel::FACE_FORWARD, CVoxel::FACE_LEFT},
            {CVoxel::FACE_BACKWARD, CVoxel::FACE_LEFT},
            {CVoxel::FACE_LEFT, CVoxel::FACE_UP},
            {CVoxel::FACE_RIGHT, CVoxel::FACE_UP}
        };

        std::vector<std::vector<CVector>> ret;
        std::map<CVector, bool> visited;

        std::queue<CVectori> voxels;

        Voxel first = m_Voxels.begin()->second;
        Voxel current = first;
        CVectori pos = first->Pos;

        std::vector<CVector> polygon;
        do
        {
            if(!voxels.empty())
            {
                pos = voxels.back();
                voxels.pop();
                current = m_Voxels[pos];
            }

            for (size_t i = 0; i < sizeof(FACES); i++)
            {
                if(FACES[i].second.Abs() == _Shape->Normal.Abs())
                    continue;

                if(((current->VisibilityMask & FACES[i].first) == FACES[i].first) || (_Shape->m_Voxels.find(pos + FACES[i].second) == _Shape->m_Voxels.end()))
                {
                    visited[pos] = true;
                    CVector shift = SHIFT_MAP.at(FACES[i].second);

                    polygon.push_back(ConvertTo2d(pos + FACES[i].second * 0.5 + shift * 0.5, _Shape->Normal));
                    polygon.push_back(ConvertTo2d(pos - FACES[i].second * 0.5 - shift * 0.5, _Shape->Normal));
                }
                else if(visited.find(pos + FACES[i].second) == visited.end())
                {
                    voxels.push(pos + FACES[i].second);
                }
            }
        } while (!voxels.empty());
        ret.push_back(polygon);

        return ret;
    }
} // namespace VoxelOptimizer
