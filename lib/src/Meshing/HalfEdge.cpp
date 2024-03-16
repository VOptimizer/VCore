/*
 * MIT License
 *
 * Copyright (c) 2024 Christian Tost
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

#include <VCore/Meshing/HalfEdge.hpp>
#include <algorithm>

namespace VCore
{
    constexpr static int GRID_CELL_SIZE = 10;

    Mesh CHalfMesh::Build()
    {
        auto ret = std::make_shared<SMesh>();
        SSurface surface;

        for (auto &&face : m_Faces)
        {
            for (char i = 0; i < 3; i++)
            {
                auto it = face->Vertices[i]->Indices.find(face->GetHash());
                if(!it->second.second)
                {
                    surface.AddVertex(SVertex(face->Vertices[i]->Pos, face->Normal, face->UV));
                    it->second.second = true;
                }

                surface.Indices.push_back(it->second.first);
            }
            
        }

        ret->Surfaces.emplace_back(std::move(surface));
        
        Clear();
        return ret;
    }

    void CHalfMesh::AddFace(const SVertex &_v1, const SVertex &_v2, const SVertex &_v3)
    {
        auto vh1 = FindOrAddVertex(_v1.Pos);
        auto vh2 = FindOrAddVertex(_v2.Pos);
        auto vh3 = FindOrAddVertex(_v3.Pos);

        // Check if there is already a vertex handle, otherwise create one.
        // if(!vh1)
        // {
        //     vh1 = new CHalfVertex(_v1.Pos);
        //     AddVertex(vh1);
        // }

        // if(!vh2)
        // {
        //     vh2 = new CHalfVertex(_v2.Pos);
        //     AddVertex(vh2);
        // }

        // if(!vh3)
        // {
        //     vh3 = new CHalfVertex(_v3.Pos);
        //     AddVertex(vh3);
        // }

        auto face = new CHalfFace(_v1.Normal, _v1.UV);
        face->Vertices[0] = vh1;
        face->Vertices[1] = vh2;
        face->Vertices[2] = vh3;

        CheckHash(vh1, face->GetHash());
        CheckHash(vh2, face->GetHash());
        CheckHash(vh3, face->GetHash());
        
        m_Faces.push_back(face);
    }

    void CHalfMesh::CheckHash(CHalfVertex* _v, uint64_t _Hash)
    {
        auto it = _v->Indices.find(_Hash);
        if(it == _v->Indices.end())
            _v->Indices[_Hash] = {m_IndexCounter++, false};
    }

    void CHalfMesh::Clear()
    {
        // for (auto &&face : m_Faces)
        //     delete face;
        
        // for (auto &&vl : m_Vertices)
        // {
        //     for (auto &&v : vl.second)
        //         delete v;
        // }
        
        // m_Faces.clear();
        // m_Vertices.clear();
        // m_IndexCounter = 0;
    }

    CHalfVertex* CHalfMesh::FindOrAddVertex(const Math::Vec3f &_Pos)
    {
        CHalfVertex *result = nullptr;
        
        int gridIdx = (int)(_Pos.x / GRID_CELL_SIZE) ^ (int)(_Pos.y / GRID_CELL_SIZE) ^ (int)(_Pos.z / GRID_CELL_SIZE);
        auto it = m_Vertices.find(gridIdx);
        if(it != m_Vertices.end())
        {
            // auto &cell = m_Vertices[gridIdx];
            // auto vit = std::find_if(cell.begin(), cell.end(), [_Pos](const CHalfVertex *_value) {
            //     return _value->Pos == _Pos;
            // });

            auto vit = std::find_if(it->second.begin(), it->second.end(), [_Pos](const CHalfVertex *_value) {
                return _value->Pos == _Pos;
            });

            if(vit != it->second.end())
                result = *vit;
            else
            {
                result = new CHalfVertex(_Pos);
                it->second.push_back(result);
            }
        }
        else
        {
            result = new CHalfVertex(_Pos);
            m_Vertices[gridIdx] = std::vector<CHalfVertex*>({result});
        }

        return result;
    }

    void CHalfMesh::AddVertex(CHalfVertex* _v)
    {
        // int gridIdx = (int)(_v->Pos.x / GRID_CELL_SIZE) ^ (int)(_v->Pos.y / GRID_CELL_SIZE) ^ (int)(_v->Pos.z / GRID_CELL_SIZE);
        // auto it = m_Vertices.find(gridIdx);
        // if(it != m_Vertices.end())
        //     it->second.push_back(_v);
        // else
        //     m_Vertices.emplace(std::make_pair(gridIdx, std::vector<CHalfVertex*>({_v})));
    }
} // namespace VCore
