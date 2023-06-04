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

#define IsFaceVisible(v, f) ((v->VisibilityMask & f) == f);

namespace VoxelOptimizer
{
    void CSlicer::SetActiveAxis(int Axis)
    {
        m_Axis = Axis;

        m_Neighbour = Math::Vec3i();
        m_Neighbour.v[Axis] = 1;
    }

    bool CSlicer::IsFace(Math::Vec3i Pos)
    {
        if(std::any_of(m_ProcessedQuads.begin(), m_ProcessedQuads.end(), 
        [this, Pos](std::pair<Math::Vec3i, Math::Vec3i> Rect)
        {
            int X = (m_Axis + 2) % 3;
            int Y = (m_Axis + 1) % 3;

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
        bool current = true;
        m_HasFace = false;

        V1 = GetVoxel(Pos);
        if(!V1)
        {
            V1 = GetVoxel(Pos + m_Neighbour);
            current = false;
        }

        if(V1)
        {
            m_Color = V1->Color;
            m_Material = V1->Material;
            SetFaceNormal(V1, current);
        }

        return m_HasFace;
    }

    Voxel CSlicer::GetVoxel(const Math::Vec3i &v)
    {
        if(!CBBox(m_TotalBBox.Beg, m_TotalBBox.Beg + m_Size).ContainsPoint(v))
            return nullptr;

        return m_Chunk->findVisible(v, CBBox(m_TotalBBox.Beg, m_Size), m_Opaque);
    }

    void CSlicer::AddProcessedQuad(Math::Vec3i Pos, Math::Vec3i Size)
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
                {
                    m_Normal = Math::Vec3i::RIGHT;
                    m_HasFace = IsFaceVisible(v, CVoxel::Visibility::RIGHT);
                } //v->Normals[CVoxel::RIGHT];
                else
                {
                    m_Normal = Math::Vec3i::LEFT;
                    m_HasFace = IsFaceVisible(v, CVoxel::Visibility::LEFT);
                }
                    //v->Normals[CVoxel::LEFT];
            }break;

            case 1: // Y-Axis
            {
                if(IsCurrent)
                {
                    m_Normal = Math::Vec3i::UP;
                    m_HasFace = IsFaceVisible(v, CVoxel::Visibility::UP);
                }//v->Normals[CVoxel::UP];
                else
                {
                    m_Normal = Math::Vec3i::DOWN;
                    m_HasFace = IsFaceVisible(v, CVoxel::Visibility::DOWN);
                }
            }break;

            case 2: // Z-Axis
            {
                if(IsCurrent)
                {
                    m_Normal = Math::Vec3i::FRONT;
                    m_HasFace = IsFaceVisible(v, CVoxel::Visibility::FORWARD);
                }//v->Normals[CVoxel::FORWARD];
                else
                {
                    m_Normal = Math::Vec3i::BACK;
                    m_HasFace = IsFaceVisible(v, CVoxel::Visibility::BACKWARD);
                }//v->Normals[CVoxel::BACKWARD];
            }break;
        }
    }
}
