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

#include "Slicer.hpp"

namespace VoxelOptimizer
{
    void CSlicer::SetActiveAxis(int Axis)
    {
        m_Axis = Axis;

        m_Neighbour = CVector();
        m_Neighbour.v[Axis] = 1;
    }

    bool CSlicer::IsFace(CVector Pos)
    {
        if(std::any_of(m_ProcessedQuads.begin(), m_ProcessedQuads.end(), 
        [this, Pos](std::pair<CVector, CVector> Rect)
        {
            int X = (m_Axis + 1) % 3;
            int Y = (m_Axis + 2) % 3;

            // Checks if Pos intersects with a already processed quad.
            if(Pos.v[X] >= Rect.first.v[X] && 
               Pos.v[X] < Rect.first.v[X] + Rect.second.v[X] &&
               Pos.v[Y] >= Rect.first.v[Y] && 
               Pos.v[Y] < Rect.first.v[Y] + Rect.second.v[Y])
               return true;

            return false;
        }))
            return false;

        Voxel V1, V2;
        CVector Size = m_Mesh->GetSize();

        V1 = GetVoxel(Pos);//m_Mesh->GetVoxel(Pos, m_Opaque);
        V2 = GetVoxel(Pos + m_Neighbour);//m_Mesh->GetVoxel(Pos + m_Neighbour, m_Opaque);

        if(!m_Opaque)
        {
            // Check if neighbour is same as current
            if(V1 && V2)
            {
                if(V1->Color != V2->Color || V1->Material != V2->Material)
                    V2 = nullptr;
            }
        }
        
        bool BlockCurrent = 0 <= Pos.v[m_Axis] ? ((bool)V1) : false;
        bool BlockCompare = Pos.v[m_Axis] < Size.v[m_Axis] - 1 ? ((bool)V2) : false;

        if(!BlockCurrent && BlockCompare)
        {
            m_Color = V2->Color;
            m_Material = V2->Material;
            SetFaceNormal(V2, false);
        }
        else if(BlockCurrent && !BlockCompare)
        {
            m_Color = V1->Color;
            m_Material = V1->Material;
            SetFaceNormal(V1, true);
        }
    
        // Checks if there is a visible face.
        return BlockCurrent != BlockCompare;
    }

    Voxel CSlicer::GetVoxel(const CVector &v)
    {
        auto it = m_Voxels.find(v);
        if(it == m_Voxels.end())
            return nullptr;

        return it->second;
    }

    void CSlicer::AddProcessedQuad(CVector Pos, CVector Size)
    {
        m_ProcessedQuads.push_back({Pos, Size});
    }

    void CSlicer::ClearQuads()
    {
        m_ProcessedQuads.clear();
    }

    void CSlicer::SetFaceNormal(Voxel v, bool IsCurrent)
    {
        // Determines the normal direction.
        switch (m_Axis)
        {
            case 0: // X-Axis
            {
                if(IsCurrent)
                    m_Normal = CVector(1, 0, 0); //v->Normals[CVoxel::RIGHT];
                else
                    m_Normal = CVector(-1, 0, 0);//v->Normals[CVoxel::LEFT];
            }break;

            case 1: // Y-Axis
            {
                if(IsCurrent)
                    m_Normal = CVector(0, 1, 0);//v->Normals[CVoxel::FORWARD];
                else
                    m_Normal = CVector(0, -1, 0);//v->Normals[CVoxel::BACKWARD];
            }break;

            case 2: // Z-Axis
            {
                if(IsCurrent)
                    m_Normal = CVector(0, 0, 1);//v->Normals[CVoxel::UP];
                else
                    m_Normal = CVector(0, 0, -1);//v->Normals[CVoxel::DOWN];
            }break;
        }
    }
} // namespace VoxelOptimizer
