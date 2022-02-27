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
    
    void CVerticesReducer::CPolygon::Optimize()
    {
        SPoint *beg = m_Points.begin()->second;
        SPoint *cur = beg;
        while (true)
        {
            SPoint *a = cur->prevs.begin()->second;
            SPoint *b = cur->nexts.begin()->second;

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
                m_Pool.dealloc(cur);
            }

            cur = cur->nexts.begin()->second;
            if(cur == beg)
                break;
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

    void CVerticesReducer::GenerateTriangles(Mesh mesh)
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

    void CVerticesReducer::GeneratePoints(Mesh mesh, const CVector &_curIdx, const std::list<CVerticesReducer::Triangle> &_triangles)
    {
        for (auto &&t : _triangles)
        {
            for (auto &&i : t->Indices)
            {
                if(i != _curIdx)
                {
                    //Obtain all remaining indices
                    if(m_Points.empty())
                    {
                        m_Points.push_back(Point(mesh->Vertices[i.x - 1], i));
                        m_Points.back().CalcAngle(mesh->Vertices[_curIdx.x - 1], mesh->Normals[_curIdx.y - 1]);
                    }
                    else
                    {
                        bool found = false;

                        for (auto &&p : m_Points)
                        {
                            if(p.Index == i)
                            {
                                found = true;
                                break;
                            }
                        }    

                        if(!found)
                        {
                            m_Points.push_back(Point(mesh->Vertices[i.x - 1], i));
                            m_Points.back().CalcAngle(mesh->Vertices[_curIdx.x - 1], mesh->Normals[_curIdx.y - 1]);
                        }
                    }
                    
                    //Delete this triangle from all shared indices
                    auto it = m_Triangles.find(i);
                    if(it != m_Triangles.end())
                        it->second.remove(t);
                }
            }   
        }        

        std::sort(m_Points.begin(), m_Points.end());
    }

    Mesh CVerticesReducer::Reduce(Mesh mesh)
    {
        std::list<Triangle> newTriangles;
        GenerateTriangles(mesh);

        for (size_t i = 0; i < 10; i++)
        {
            auto trianglesIt = m_Triangles.begin();
            while (trianglesIt != m_Triangles.end())
            {
                CPolygon poly(trianglesIt->second, mesh->Vertices, mesh->Normals[trianglesIt->first.y - 1]);
                poly.Remove(trianglesIt->first);
                
                if(poly.IsClosed())
                {
                    // poly.Optimize();

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

                    // GeneratePoints(mesh, trianglesIt->first, trianglesIt->second);

                    // if(m_Points.size() < 3)
                    // {
                    //     m_Points.clear();
                        
                    //     for (auto &&triangle : trianglesIt->second)
                    //     {
                    //         for (auto &&i : triangle->Indices)
                    //         {
                    //             if(i != trianglesIt->first)
                    //             {
                    //                 //Delete this triangle from all shared indices
                    //                 auto it = m_Triangles.find(i);
                    //                 if(it != m_Triangles.end())
                    //                     it->second.push_back(triangle);                               
                    //             }
                    //         }
                    //     }

                    //     trianglesIt++;
                    //     continue;
                    // }

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
                                    //Delete this triangle from all shared indices
                                    auto it = m_Triangles.find(i);
                                    if(it != m_Triangles.end())
                                        it->second.push_back(triangle);                               
                                }
                            }
                        }

                        trianglesIt++;
                        continue;
                    }

                    // std::vector<CVector> polygon;
                    // for (auto &&p : m_Points)
                    //     polygon.push_back(p.Position2D);
                    
                    // std::vector<int> polyindices;
                    // bool res = CTriangulate::Triangulate(polygon, polyindices);
                    // if(res)
                    // {
                    //     for (size_t i = 0; i < polyindices.size(); i += 3)
                    //     {
                    //         const Point &a = m_Points[polyindices[i]];
                    //         const Point &b = m_Points[polyindices[i + 1]];
                    //         const Point &c = m_Points[polyindices[i + 2]];

                    //         auto triangle = std::make_shared<CTriangle>();
                    //         triangle->Indices.push_back(a.Index);
                    //         triangle->Indices.push_back(b.Index);
                    //         triangle->Indices.push_back(c.Index);
                    //         triangle->Mat = trianglesIt->second.front()->Mat;

                    //         // newTriangles.push_back(triangle);

                    //         m_Triangles[a.Index].push_back(triangle);
                    //         m_Triangles[b.Index].push_back(triangle);
                    //         m_Triangles[c.Index].push_back(triangle);
                    //     }
                    // }
                    // else
                    // {
                    //     m_Points.clear();
                    //     for (auto &&triangle : trianglesIt->second)
                    //     {
                    //         for (auto &&i : triangle->Indices)
                    //         {
                    //             if(i != trianglesIt->first)
                    //             {
                    //                 //Delete this triangle from all shared indices
                    //                 auto it = m_Triangles.find(i);
                    //                 if(it != m_Triangles.end())
                    //                     it->second.push_back(triangle);                               
                    //             }
                    //         }
                    //     }

                    //     trianglesIt++;
                    //     continue;
                    // }
                    
                    trianglesIt = m_Triangles.erase(trianglesIt);
                    // m_Points.clear();
                }
                else
                    trianglesIt++;
            }
        }
        
        CMeshBuilder builder;
        builder.AddTextures(mesh->Textures);

        for (auto &&t : m_Triangles)
        {
            for (auto &&triangle : t.second)
            {
                // TODO: BOOST This is slow
                if(std::find(newTriangles.begin(), newTriangles.end(), triangle) == newTriangles.end())
                    newTriangles.push_back(triangle);
            }
        }

        for (auto &&t : newTriangles)
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
        
        return builder.Build();
        
        // //Iterate over all triangles and delete all triangles which have shared indices
        // auto trianglesIt = triangles.begin();
        // while (trianglesIt != triangles.end())
        // {
        //     if (initTriangles[trianglesIt->first] % 3 == 0)
        //     {
        //         for (auto &&triangle : trianglesIt->second)
        //         {
        //             for (auto &&i : triangle->Indices)
        //             {
        //                 //Reduce the vertex count
        //                 verticesCounter[i.x - 1]--;

        //                 if(i != trianglesIt->first)
        //                 {
        //                     //Obtain all remaining indices
        //                     if(m_Points.empty())
        //                     {
        //                         m_Points.push_back(Point(mesh->Vertices[i.x - 1], i));
        //                         m_Points.back().CalcAngle(mesh->Vertices[trianglesIt->first.x - 1], mesh->Normals[trianglesIt->first.y - 1]);
        //                     }
        //                     else
        //                     {
        //                         bool found = false;

        //                         for (auto &&p : m_Points)
        //                         {
        //                             if(p.Index == i)
        //                             {
        //                                 found = true;
        //                                 break;
        //                             }
        //                         }    

        //                         if(!found)
        //                         {
        //                             m_Points.push_back(Point(mesh->Vertices[i.x - 1], i));
        //                             m_Points.back().CalcAngle(mesh->Vertices[trianglesIt->first.x - 1], mesh->Normals[trianglesIt->first.y - 1]);
        //                         }
        //                     }
                            
        //                     //Delete this triangle from all shared indices
        //                     auto it = triangles.find(i);
        //                     if(it != triangles.end())
        //                         it->second.remove(triangle);
        //                 }
        //             }
        //         }

        //         std::sort(m_Points.begin(), m_Points.end());
        //         if(m_Points.size() < 3)
        //         {
        //             m_Points.clear();

        //             for (auto &&triangle : trianglesIt->second)
        //             {
        //                 for (auto &&i : triangle->Indices)
        //                 {
        //                     //Reduce the vertex count
        //                     verticesCounter[i.x - 1]++;

        //                     if(i != trianglesIt->first)
        //                     {
        //                         //Delete this triangle from all shared indices
        //                         auto it = triangles.find(i);
        //                         if(it != triangles.end())
        //                             it->second.push_back(triangle);                               
        //                     }
        //                 }
        //             }

        //             trianglesIt++;
                    
        //             continue;
        //         }

        //         std::vector<CVector> polygon;

        //         cout << "\n\n\n\n" << endl;
        //         for (auto &&p : m_Points)
        //         {
        //             polygon.push_back(p.Position2D);
        //             // cout << "Position3D: " << p.Position << " Position2D: " << p.Position2D << endl;
        //         }
                
        //         std::vector<int> polyindices;

        //         bool res = CTriangulate::Triangulate(polygon, polyindices);
        //         if(res)
        //         {
        //             for (size_t i = 0; i < polyindices.size(); i += 3)
        //             {
        //                 const Point &a = m_Points[polyindices[i]];
        //                 const Point &b = m_Points[polyindices[i + 1]];
        //                 const Point &c = m_Points[polyindices[i + 2]];

        //                 auto triangle = std::make_shared<CTriangle>();
        //                 triangle->Indices.push_back(a.Index);
        //                 triangle->Indices.push_back(b.Index);
        //                 triangle->Indices.push_back(c.Index);

        //                 newTriangles.push_back(triangle);

        //                 // triangles[a.Index].push_back(triangle);
        //                 // triangles[b.Index].push_back(triangle);
        //                 // triangles[c.Index].push_back(triangle);

        //                 verticesCounter[a.Index.x - 1]++;
        //                 verticesCounter[b.Index.x - 1]++;
        //                 verticesCounter[c.Index.x - 1]++;
        //             }
        //         }
        //         else
        //         {
        //             for (auto &&triangle : trianglesIt->second)
        //             {
        //                 for (auto &&i : triangle->Indices)
        //                 {
        //                     //Reduce the vertex count
        //                     verticesCounter[i.x - 1]++;

        //                     if(i != trianglesIt->first)
        //                     {
        //                         //Delete this triangle from all shared indices
        //                         auto it = triangles.find(i);
        //                         if(it != triangles.end())
        //                             it->second.push_back(triangle);                               
        //                     }
        //                 }
        //             }

        //             trianglesIt++;

        //             int ddd = 0;
        //             ddd++;
        //             continue;
        //         }
                
        //         trianglesIt = triangles.erase(trianglesIt);
        //         m_Points.clear();
        //     }
        //     else
        //         trianglesIt++;
        // }
    
        // std::vector<int> verticesToRemove;
        // int sub = 0;

        // for (size_t i = 0; i < verticesCounter.size(); i++)
        // {
        //     if (verticesCounter[i] <= 0)
        //     {
        //         mesh->Vertices.erase(mesh->Vertices.begin() + (i - sub));
        //         sub++;
        //         verticesToRemove.push_back(i + 1);
        //     }
        // }  

        // std::sort(verticesToRemove.begin(), verticesToRemove.end());

        // for (auto &&t : triangles)
        // {
        //     for (auto &&triangle : t.second)
        //     {
        //         newTriangles.push_back(triangle);
        //     }
        // }

        // std::vector<CVector> indices;
        // for (auto &&triangle : newTriangles)
        // {
        //     for (auto &&i : triangle->Indices)
        //     {
        //         CVector index = i;

        //         for (auto &&r : verticesToRemove)
        //         {
        //             if (i.x > r)
        //                 index.x--;
        //             else
        //                 break;
        //         }

        //         indices.push_back(index);
        //     }
        // }

        // // for (auto &&t : triangles)
        // // {
        // //     for (auto &&triangle : t.second)
        // //     {
        // //         for (auto &&i : triangle->Indices)
        // //         {
        // //             CVector index = i;

        // //             for (auto &&r : verticesToRemove)
        // //             {
        // //                 if (i.x > r)
        // //                     index.x--;
        // //                 else
        // //                     break;
        // //             }

        // //             indices.push_back(index);

        // //             if(i != t.first)
        // //             {
        // //                 //Delete this triangle from all shared indices
        // //                 auto it = triangles.find(i);
        // //                 if(it != triangles.end())
        // //                     it->second.remove(triangle);
        // //             }
        // //         }
        // //     }
        // // }

        // mesh->Faces.front()->Indices = indices;

        // return mesh;
    }
} // namespace VoxelOptimizer
