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

#include <map>
#include <VoxelOptimizer/Voxel/VoxelMesh.hpp>

namespace VoxelOptimizer
{
    void CVoxelMesh::SetVoxel(const Math::Vec3i &Pos, int Material, int Color, bool Transparent, CVoxel::Visibility mask)
    {      
        CVoxel Tmp;// = m_Pool.alloc();
        
        Tmp.Material = Material;
        Tmp.Color = Color;
        Tmp.Transparent = Transparent;
        Tmp.VisibilityMask = mask;

        m_Voxels.insert({Pos, Tmp});
    }

    void CVoxelMesh::RemoveVoxel(const Math::Vec3i &Pos)
    {
        auto IT = m_Voxels.find(Pos);
        if(IT != m_Voxels.end())
        {
            // SetNormal(Pos, CVoxel::FACE_UP, false);
            // SetNormal(Pos, CVoxel::FACE_DOWN, false);

            // SetNormal(Pos, CVoxel::FACE_LEFT, false);
            // SetNormal(Pos, CVoxel::FACE_RIGHT, false);

            // SetNormal(Pos, CVoxel::FACE_FORWARD, false);
            // SetNormal(Pos, CVoxel::FACE_BACKWARD, false);

            m_Voxels.erase(IT);
            // MarkChunk(Pos);
        }
    }
    
    void CVoxelMesh::Clear()
    {
        
        m_Voxels.clear();
    }

    Voxel CVoxelMesh::GetVoxel(const Math::Vec3i &Pos)
    {
        auto it = m_Voxels.find(Pos);
        if(it == m_Voxels.end())
            return nullptr;

        return it->second;
    }

    Voxel CVoxelMesh::GetVoxel(const Math::Vec3i &Pos, bool OpaqueOnly)
    {
        auto it = m_Voxels.find(Pos, OpaqueOnly);
        if(it == m_Voxels.end())
            return nullptr;

        return it->second;
    }

    Voxel CVoxelMesh::GetVisibleVoxel(const Math::Vec3i &Pos)
    {
        auto it = m_Voxels.findVisible(Pos);
        if(it == m_Voxels.end())
            return nullptr;

        return it->second;
    }

    Voxel CVoxelMesh::GetVisibleVoxel(const Math::Vec3i &Pos, bool OpaqueOnly)
    {
        auto it = m_Voxels.findVisible(Pos, OpaqueOnly);
        if(it == m_Voxels.end())
            return nullptr;

        return it->second;
    }

    void CVoxelMesh::GenerateVisibilityMask()
    {
        m_Voxels.generateVisibilityMask();
    }

    VectoriMap<Voxel> CVoxelMesh::QueryVisible(bool opaque) const
    {
        return m_Voxels.queryVisible(opaque);
    }

    std::vector<SChunkMeta> CVoxelMesh::QueryDirtyChunks(const CFrustum *_Frustum)
    {
        return m_Voxels.queryDirtyChunks(_Frustum);
    }

    std::vector<SChunkMeta> CVoxelMesh::QueryChunks(const CFrustum *_Frustum) const
    {
        return m_Voxels.queryChunks(_Frustum);
    }
    
    void CVoxelMesh::SetNormal(const Math::Vec3i &Pos, const Math::Vec3i &Neighbor, bool IsInvisible)
    {
        // static const std::map<Math::Vec3f, std::pair<CVoxel::Visibility, CVoxel::Visibility>> NEIGHBOR_INDEX = {
        //     {CVoxel::FACE_UP, {CVoxel::Visibility::UP, CVoxel::Visibility::DOWN}},
        //     {CVoxel::FACE_DOWN, {CVoxel::Visibility::DOWN, CVoxel::Visibility::UP}},

        //     {CVoxel::FACE_LEFT, {CVoxel::Visibility::LEFT, CVoxel::Visibility::RIGHT}},
        //     {CVoxel::FACE_RIGHT, {CVoxel::Visibility::RIGHT, CVoxel::Visibility::LEFT}},

        //     {CVoxel::FACE_FORWARD, {CVoxel::Visibility::FORWARD, CVoxel::Visibility::BACKWARD}},
        //     {CVoxel::FACE_BACKWARD, {CVoxel::Visibility::BACKWARD, CVoxel::Visibility::FORWARD}},
        // };

        // Voxel cur = GetVoxel(Pos);
        // if(cur == nullptr)
        //     return;

        // Voxel neighbor = GetVoxel(Pos + Neighbor);
        // auto visibility = NEIGHBOR_INDEX.at(Neighbor);

        // // 1. Both opaque touching faces invisible
        // // 2. One transparent touching faces visible
        // // 3. Both transparent different transparency faces visible
        // // 4. Both transparent same transparency and color faces invisible
        // // 5. Both transparent different transparency and same color faces visible

        // if(neighbor)
        // {
        //     bool hideFaces = false;

        //     if(!neighbor->Transparent && !cur->Transparent)
        //         hideFaces = true;
        //     else if(neighbor->Transparent && !cur->Transparent || !neighbor->Transparent && cur->Transparent)
        //         hideFaces = false;
        //     else if(neighbor->Transparent && cur->Transparent)
        //     {
        //         if(neighbor->Material != cur->Material)
        //             hideFaces = false;
        //         else if(neighbor->Color != cur->Color)
        //             hideFaces = false;
        //         else
        //             hideFaces = true;
        //     }

        //     if(hideFaces)
        //     {
        //         neighbor->VisibilityMask = IsInvisible ? (CVoxel::Visibility)(neighbor->VisibilityMask & ~visibility.second) : (CVoxel::Visibility)(neighbor->VisibilityMask | visibility.second);
        //         cur->VisibilityMask = IsInvisible ? (CVoxel::Visibility)(cur->VisibilityMask & ~visibility.first) : (CVoxel::Visibility)(cur->VisibilityMask | visibility.first);
        //     }
        //     else
        //         cur->VisibilityMask = (CVoxel::Visibility)(cur->VisibilityMask | visibility.first);
        // }
        // else
        //     cur->VisibilityMask = (CVoxel::Visibility)(cur->VisibilityMask | visibility.first);

        // // CheckInvisible(cur);
        // // if(neighbor)
        //     CheckInvisible(neighbor);
    }
}
