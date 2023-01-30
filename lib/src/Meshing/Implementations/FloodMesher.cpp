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

#include <fstream>

namespace VoxelOptimizer
{
    class PolygonDebugger
    {
        public:
            PolygonDebugger() 
            {
                out.open("points.txt");
            }

            void WritePoints(const std::vector<CVector> &polygon)
            {
                for (auto &&p : polygon)
                    out << p.x << "," << p.y << "\n";                
            }

            ~PolygonDebugger()
            {
                out.close();
            }

        private:
            std::ofstream out;
    };

    static const std::pair<CVoxel::Visibility, CVector> FACES[] = {
        {CVoxel::Visibility::UP, CVoxel::FACE_UP},
        {CVoxel::Visibility::DOWN, CVoxel::FACE_DOWN},
        {CVoxel::Visibility::FORWARD, CVoxel::FACE_FORWARD},
        {CVoxel::Visibility::BACKWARD, CVoxel::FACE_BACKWARD},
        {CVoxel::Visibility::LEFT, CVoxel::FACE_LEFT},
        {CVoxel::Visibility::RIGHT, CVoxel::FACE_RIGHT}
    };

    static const int FACES_SIZE = sizeof(FACES) / sizeof(FACES[0]);

    std::vector<CVector>::iterator FindPosition(std::vector<CVector> &_Polygon, const CVector &_Vec)
    {
        auto it = _Polygon.begin();
        while (it != _Polygon.end())
        {
            if(*it == _Vec)
                break;

            it++;
        }
        return it;
    }

    CVector ConvertTo2d(const CVector &_Vec, const CVector &_Normal)
    {
        CVector ret;
        int retI = 0;
        for (size_t n = 0; n < 3; n++)
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
        for (size_t n = 0; n < 3; n++)
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
        for(auto &&shape : m_Shapes)
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
                m_Visited[pos] = true;

                voxels.pop();
                current = m_Voxels[pos];
            }

            for (size_t i = 0; i < FACES_SIZE; i++)
            {
                if((current->VisibilityMask & FACES[i].first) == FACES[i].first)
                {
                    int layer = (pos * FACES[i].second.Abs()).Sum();

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
                    if(m_Visited.find(newPos) == m_Visited.end())
                    {
                        Voxel other = m_Voxels[newPos];
                        if(other->Material == first->Material && other->Color == first->Color && other->Transparent == first->Transparent)
                            voxels.push(newPos);
                        else
                            m_Colors.push(newPos);
                    }
                }
            }
        } while (!voxels.empty());

        for(auto &&face : faces)
        {
            for(auto &&layer : face.second)
            {
                m_Shapes.push_back(layer.second);
            }
        }
    }

    std::vector<std::vector<CVector>> CFloodMesher::GeneratePolygons(SFloodShape *_Shape)
    {
        std::vector<std::vector<CVector>> ret;
        std::map<CVector, bool> visited;

        std::queue<CVectori> voxels;

        Voxel first = _Shape->m_Voxels.begin()->second;
        Voxel current = first;
        CVector pos = first->Pos;

        std::ofstream out("voxels.txt", std::ios::out);

        std::vector<CVector> polygon;
        do
        {
            if(!voxels.empty())
            {
                pos = voxels.back();
                voxels.pop();
                current = _Shape->m_Voxels[pos];
            }

            out << pos.x << ","<<pos.y<<","<<pos.z<<"\n";

            for (size_t i = 0; i < FACES_SIZE; i++)
            {
                if(FACES[i].second.Abs() == _Shape->Normal.Abs())
                    continue;

                if(((current->VisibilityMask & FACES[i].first) == FACES[i].first) || (_Shape->m_Voxels.find(pos + FACES[i].second) == _Shape->m_Voxels.end()))
                {
                    visited[pos] = true;
                    CVector v1, v2;
                    CVector pos2d = ConvertTo2d(pos, _Shape->Normal);

                    switch (FACES[i].first)
                    {
                        case CVoxel::Visibility::UP:
                        {
                            v1 = pos + CVector(-0.5, 0, 0.5);
                            v2 = pos + CVector(0.5, 0, 0.5);
                        }break;
                    
                        case CVoxel::Visibility::DOWN:
                        {
                            v1 = pos + CVector(-0.5, 0, -0.5);
                            v2 = pos + CVector(0.5, 0, -0.5);
                        }break;

                        case CVoxel::Visibility::LEFT:
                        {
                            v1 = pos + CVector(-0.5, 0, 0.5);
                            v2 = pos + CVector(-0.5, 0, -0.5);
                        }break;

                        case CVoxel::Visibility::RIGHT:
                        {
                            v1 = pos + CVector(0.5, 0, 0.5);
                            v2 = pos + CVector(0.5, 0, -0.5);
                        }break;

                        case CVoxel::Visibility::FORWARD:
                        {
                            v1 = pos + CVector(-0.5, 0.5, 0);
                            v2 = pos + CVector(0.5, 0.5, 0);
                        }break;

                        case CVoxel::Visibility::BACKWARD:
                        {
                            v1 = pos + CVector(-0.5, -0.5, 0);
                            v2 = pos + CVector(0.5, -0.5, 0);
                        }break;
                    }
                    // CVector shift = SHIFT_MAP.at(FACES[i].second);

                    v1 = ConvertTo2d(v1, _Shape->Normal);
                    v2 = ConvertTo2d(v2, _Shape->Normal);

                    auto it1 = FindPosition(polygon, v1);
                    auto it2 = FindPosition(polygon, v2);

                    if(it2 != polygon.end() && it1 == polygon.end())
                        polygon.insert(it2, v1);
                    else if(it2 == polygon.end() && it1 != polygon.end())
                        polygon.insert(it1, v2);
                    else if(it2 == polygon.end() && it1 == polygon.end())
                    {
                        polygon.push_back(v1);

                        if(v1 != v2)
                            polygon.push_back(v2);
                    }

                    // polygon.push_back(ConvertTo2d(pos + FACES[i].second * 0.5 + shift * 0.5, _Shape->Normal));
                    // polygon.push_back(ConvertTo2d(pos - FACES[i].second * 0.5 - shift * 0.5, _Shape->Normal));
                }
                else if(visited.find(pos + FACES[i].second) == visited.end())
                {
                    voxels.push(pos + FACES[i].second);
                }
            }
        } while (!voxels.empty());

        out.close();
        {
            PolygonDebugger dbg;
            dbg.WritePoints(polygon);
        }

        ret.push_back(polygon);

        return ret;
    }
} // namespace VoxelOptimizer
