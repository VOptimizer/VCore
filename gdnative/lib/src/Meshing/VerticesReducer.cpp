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

#include <algorithm>
#include <VoxelOptimizer/Meshing/MeshBuilder.hpp>
#include <VoxelOptimizer/Meshing/VerticesReducer.hpp>
#include <list>
#include <map>
#include <vector>
#include "Implementations/Triangulation/Triangulate.hpp"

#include <iostream>

using namespace std;

namespace VoxelOptimizer
{
    //////////////////////////////////////////////////
    // CVerticesReducer::CPolygon functions
    //////////////////////////////////////////////////

    CVector CVerticesReducer::CPolygon::CalcCenter(Triangle t, const std::vector<CVector> &_verts)
    {
        CVector ret;
        for (auto &&i : t->Indices)
        {
            ret += _verts[i.x - 1];
        }
        return ret / 3.f;
    }

    bool CVerticesReducer::CPolygon::IsClockwise(Triangle t, const std::vector<CVector> &_verts)
    {
        CVector center = CalcCenter(t, _verts);
        std::vector<CVector> polygon;
        
        for (auto &&i : t->Indices)
        {
            CVector pos = _verts[i.x - 1];
            CVector pos2d = ProjectToCube(pos);

            polygon.push_back(pos2d);          
        }

        float area = CTriangulate::Area(polygon);
        return area < 0.0f;
    }

    void CVerticesReducer::CPolygon::Remove(const CVector &_idx)
    {
        auto it = m_Points.find(_idx);
        if(it != m_Points.end())
        {
            SPoint *p = it->second;
            m_Points.erase(it);

            p->Remove();
            m_Pool.dealloc(p);
        }
    }

    size_t CalcIndex(long idx, size_t size)
    {
        if(idx < 0)
            return size - 1;
        else if(idx >= size)
            return 0;
        
        return idx;
    }
    
    void CVerticesReducer::CPolygon::Optimize()
    {
        std::vector<SPoint *> points(m_Points.size(), nullptr);
        auto center2d = ProjectToCube(m_Center);
        size_t idx = 0;
        
        for (auto &&p : m_Points)
        {
            auto pos2d = ProjectToCube(p.second->Position);
            p.second->Angle = atan2(pos2d.y - center2d.y, pos2d.x - center2d.x);
            points[idx++] = p.second;
            p.second->nexts.clear();
            p.second->prevs.clear();
        }

        std::sort(points.begin(), points.end(), [](SPoint *a, SPoint *b) { return a->Angle < b->Angle; });

        idx = 0;
        SPoint *beg = points.front();
        SPoint *cur = beg;
        while (true)
        {
            SPoint *a = points[CalcIndex(idx - 1, points.size())];
            SPoint *b = points[CalcIndex(idx + 1, points.size())];

            CVector a2d = ProjectToCube(a->Position);
            CVector b2d = ProjectToCube(cur->Position);
            CVector c2d = ProjectToCube(b->Position);

            float cross = CTriangulate::Cross2D(b2d - a2d, c2d - a2d);
            if(cross == 0)
            {
                cur->Remove();
                a->AddNext(b);
                b->AddPrev(a);

                if(cur == beg)
                    beg = a;

                m_Points.erase(cur->Index);
                points.erase(points.begin() + idx);
                m_Pool.dealloc(cur);

                idx = CalcIndex(idx - 1, points.size());
                cur = a;
            }
            else
            {
                idx = CalcIndex(idx + 1, points.size());
                cur = points[idx];
                if(cur == beg)
                    break;
            }
        }      

        if(!points.empty())
        {
            if(points.back()->nexts.empty())
            {
                points.back()->AddNext(points.front());
                points.front()->AddPrev(points.back());
            }
        }
    }
    
    std::list<CVerticesReducer::Triangle> CVerticesReducer::CPolygon::Triangulate()
    {
        std::vector<CVector> polygon;
        std::vector<SPoint*> points(m_Points.size(), nullptr);
        std::list<CVerticesReducer::Triangle> ret;

        size_t idx = 0;

        SPoint *beg = m_Points.begin()->second;
        SPoint *cur = beg;
        while (true)
        {
            polygon.push_back(ProjectToCube(cur->Position));
            points[idx++] = cur;

            cur = cur->prevs.begin()->second;
            if(cur == beg)
                break;
        }
        
        std::vector<int> polyindices;
        bool res = CTriangulate::Triangulate(polygon, polyindices);
        if(res)
        {
            for (size_t i = 0; i < polyindices.size(); i += 3)
            {
                const SPoint *a = points[polyindices[i]];
                const SPoint *b = points[polyindices[i + 1]];
                const SPoint *c = points[polyindices[i + 2]];

                auto triangle = std::make_shared<CTriangle>();
                triangle->Indices.push_back(a->Index);
                triangle->Indices.push_back(b->Index);
                triangle->Indices.push_back(c->Index);
                triangle->Mat = a->Mat;

                ret.push_back(triangle);
            }
        }

        return ret;
    }

    CVerticesReducer::CPolygon::CPolygon(const std::list<Triangle> &_tris, const std::vector<CVector> &_verts, const CVector &normal)
    {
        if(_tris.empty())
            return;

        std::map<CVector, SPoint*> points;
        SPoint *first = nullptr, *last = nullptr;
        m_Normal = normal;

        bool clockwise = IsClockwise(_tris.front(), _verts);
        for (auto &&t : _tris)
        {
            for (auto &&i : t->Indices)
            {
                SPoint *cur;

                auto it = points.find(i);
                if(it != points.end())
                    cur = it->second;
                else
                {
                    cur = m_Pool.alloc(_verts[i.x - 1]);
                    cur->Index = i;
                    cur->Mat = t->Mat;

                    m_Points[cur->Index] = cur;
                    points.insert({i, cur});
                    m_Center += cur->Position;
                }

                if(clockwise && last)
                {
                    last->AddNext(cur);
                    cur->AddPrev(last);
                }
                else if(last)
                {
                    last->AddPrev(cur);
                    cur->AddNext(last);
                }

                if(!first)
                    first = cur;
                last = cur;
            }

            if(clockwise)
            {
                last->AddNext(first);
                first->AddPrev(last);
            }
            else
            {
                last->AddPrev(first);
                first->AddNext(last);
            }
            
            first = last = nullptr;
        }

        m_Center = m_Center / (float)m_Points.size();
    }

    bool CVerticesReducer::CPolygon::IsClosed()
    {
        if(m_Points.size() < 3)
            return false;

        SPoint *beg = m_Points.begin()->second;
        SPoint *cur = beg;
        while (true)
        {
            if(cur->nexts.size() == 1)
                cur = cur->nexts.begin()->second;
            else
                return false;

            if(cur == beg)
                break;
        }

        return true;
    }

    CVector CVerticesReducer::CPolygon::ProjectToCube(const CVector &_position)
    {
        CVector ret;

        int isXPositive = m_Normal.x > 0 ? 1 : 0;
        int isYPositive = m_Normal.y > 0 ? 1 : 0;
        int isZPositive = m_Normal.z > 0 ? 1 : 0;

        float absX = fabs(m_Normal.x);
        float absY = fabs(m_Normal.y);
        float absZ = fabs(m_Normal.z);
                
        // Positive X
        if (isXPositive && absX >= absY && absX >= absZ) 
        {
            ret.x = -_position.z;
            ret.y = _position.y;
        }
        // Negative X
        if (!isXPositive && absX >= absY && absX >= absZ) 
        {
            ret.x = _position.z;
            ret.y = _position.y;
        }
        // Positive Y
        if (isYPositive && absY >= absX && absY >= absZ) 
        {
            ret.x = _position.x;
            ret.y = -_position.z;
        }
        // Negative Y
        if (!isYPositive && absY >= absX && absY >= absZ) 
        {
            ret.x = _position.x;
            ret.y = _position.z;
        }
        // Positive Z
        if (isZPositive && absZ >= absX && absZ >= absY) 
        {
            ret.x = _position.x;
            ret.y = _position.y;
        }
        // Negative Z
        if (!isZPositive && absZ >= absX && absZ >= absY) 
        {
            ret.x = -_position.x;
            ret.y = _position.y;
        }

        return ret;
    }

    //////////////////////////////////////////////////
    // CVerticesReducer::CPolygon::SPoint functions
    //////////////////////////////////////////////////

    void CVerticesReducer::CPolygon::SPoint::AddNext(SPoint* p)
    {
        auto it = nexts.find(p->Position);
        if(it == nexts.end())
            nexts.insert({p->Position, p});
    }

    void CVerticesReducer::CPolygon::SPoint::AddPrev(SPoint* p)
    {
        auto it = prevs.find(p->Position);
        if(it == prevs.end())
            prevs.insert({p->Position, p});
    }

    void CVerticesReducer::CPolygon::SPoint::Remove()
    {
        for (auto &&p : prevs)
        {
            p.second->Remove(this);
        }
        
        for (auto &&p : nexts)
        {
            p.second->Remove(this);
        }
    }

    void CVerticesReducer::CPolygon::SPoint::Remove(SPoint* p)
    {
        nexts.erase(p->Position);
        prevs.erase(p->Position);
    }

    //////////////////////////////////////////////////
    // CVerticesReducer functions
    //////////////////////////////////////////////////

    void CVerticesReducer::GenerateTriangles(const Mesh &mesh)
    {
        m_Triangles.clear();

        // Builds a map of indices to triangles.
        for (auto &&f : mesh->Faces)
        {
            for (size_t i = 0; i < f->Indices.size(); i += 3)
            {
                // Create a new triangle
                auto triangle = std::make_shared<CTriangle>();
                triangle->Indices.push_back(f->Indices[i]);
                triangle->Indices.push_back(f->Indices[i + 1]);
                triangle->Indices.push_back(f->Indices[i + 2]);
                triangle->Mat = f->FaceMaterial;

                // Adds the new triangle to the list of each index.
                m_Triangles[f->Indices[i]].push_back(triangle);
                m_Triangles[f->Indices[i + 1]].push_back(triangle);
                m_Triangles[f->Indices[i + 2]].push_back(triangle);
            }
        }
    }

    void CVerticesReducer::ReduceTriangles(const Mesh &mesh)
    {
        auto trianglesIt = m_Triangles.begin();
        while (trianglesIt != m_Triangles.end())
        {
            CPolygon poly(trianglesIt->second, mesh->Vertices, mesh->Normals[trianglesIt->first.y - 1]);
            poly.Remove(trianglesIt->first);
            
            if(poly.IsClosed())
            {
                for (auto &&t : trianglesIt->second)
                {
                    for (auto &&i : t->Indices)
                    {
                        if(i != trianglesIt->first)
                        {
                            //Delete this triangle from all shared indices
                            auto it = m_Triangles.find(i);
                            if(it != m_Triangles.end())
                            {
                                it->second.remove(t);
                                if(it->second.empty())
                                    m_Triangles.erase(it);
                            }
                        }
                    }
                }

                auto tris = poly.Triangulate();
                if(!tris.empty())
                {
                    for (auto &&t : tris)
                    {
                        const CVector &a = t->Indices[0];
                        const CVector &b = t->Indices[1];
                        const CVector &c = t->Indices[2];

                        m_Triangles[a].push_back(t);
                        m_Triangles[b].push_back(t);
                        m_Triangles[c].push_back(t);
                    }
                }
                else
                {
                    for (auto &&triangle : trianglesIt->second)
                    {
                        for (auto &&i : triangle->Indices)
                        {
                            if(i != trianglesIt->first)
                            {
                                //Adds all triangles back.
                                auto it = m_Triangles.find(i);
                                if(it != m_Triangles.end())
                                    it->second.push_back(triangle);                               
                            }
                        }
                    }

                    trianglesIt++;
                    continue;
                }
                
                trianglesIt = m_Triangles.erase(trianglesIt);
            }
            else
                trianglesIt++;
        }
    }

    void CVerticesReducer::ReduceVertices(const Mesh &mesh)
    {
        std::list<CVector> indices;
        std::list<Triangle> tris;
        for (auto &&t : m_Triangles)
        {
            auto list = t.second;
            while (!list.empty())
            {
                for (auto &&tri : list)
                {
                    if(!tri->Processed)
                    {
                        tris.push_back(tri);
                        for (auto &&i : tri->Indices)
                        {
                            if(i != t.first)
                                indices.push_back(i);
                        }

                        tri->Processed = true;
                    }
                }

                if(indices.empty())
                    break;

                list = m_Triangles[indices.front()];
                indices.erase(indices.begin());
            }

            if(!tris.empty())
            {
                CPolygon poly(tris, mesh->Vertices, mesh->Normals[t.first.y - 1]);
                poly.Optimize();

                if(poly.IsClosed())
                {
                    auto newTris = poly.Triangulate();
                    m_NewTriangles.insert(m_NewTriangles.end(), newTris.begin(), newTris.end());
                }
                else
                    m_NewTriangles.insert(m_NewTriangles.end(), tris.begin(), tris.end());
            }

            tris.clear();
            indices.clear();
        }
    }

    Mesh CVerticesReducer::Reduce(const Mesh &mesh)
    {
        GenerateTriangles(mesh);
        ReduceTriangles(mesh);
        ReduceVertices(mesh);

        CMeshBuilder builder;
        builder.AddTextures(mesh->Textures);

        for (auto &&t : m_NewTriangles)
        {
            for (size_t i = 0; i < t->Indices.size(); i += 3)
            {
                CVector idx1 = t->Indices[i];
                CVector idx2 = t->Indices[i + 1];
                CVector idx3 = t->Indices[i + 2];

                SVertex v1 = {
                    mesh->Vertices[idx1.x - 1],
                    mesh->UVs[idx1.z - 1],
                    mesh->Normals[idx1.y - 1],
                    t->Mat,
                    0
                };

                SVertex v2 = {
                    mesh->Vertices[idx2.x - 1],
                    mesh->UVs[idx2.z - 1],
                    mesh->Normals[idx2.y - 1],
                    t->Mat,
                    0
                };

                SVertex v3 = {
                    mesh->Vertices[idx3.x - 1],
                    mesh->UVs[idx3.z - 1],
                    mesh->Normals[idx3.y - 1],
                    t->Mat,
                    0
                };

                builder.AddFace(v1, v2, v3);
            }
        }

        m_NewTriangles.clear();
        m_Triangles.clear();
        
        return builder.Build();
    }
} // namespace VoxelOptimizer
