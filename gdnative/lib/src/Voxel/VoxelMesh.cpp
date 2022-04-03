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
#include <VoxelOptimizer/Voxel/Octree.hpp>
#include <VoxelOptimizer/Voxel/VoxelMesh.hpp>

namespace VoxelOptimizer
{
    const CVector CVoxel::FACE_UP = CVector(0, 0, 1);
    const CVector CVoxel::FACE_DOWN = CVector(0, 0, -1);
    const CVector CVoxel::FACE_LEFT = CVector(-1, 0, 0);
    const CVector CVoxel::FACE_RIGHT = CVector(1, 0, 0);
    const CVector CVoxel::FACE_FORWARD = CVector(0, 1, 0);
    const CVector CVoxel::FACE_BACKWARD = CVector(0, -1, 0);
    const CVector CVoxel::FACE_ZERO = CVector(0, 0, 0);
    const CVector CVoxelMesh::CHUNK_SIZE = CVector(16, 16, 16);

    CVoxel::CVoxel()
    {
        VisibilityMask = Visibility::VISIBLE;
    }

    void CVoxelMesh::SetVoxel(const CVector &Pos, int Material, int Color, bool Transparent, CVoxel::Visibility mask)
    {
        //std::lock_guard<std::recursive_mutex> lock(m_Lock);

        std::chrono::steady_clock::time_point begin1 = std::chrono::steady_clock::now();
        Voxel Tmp = m_Pool.alloc();
        std::chrono::steady_clock::time_point end1 = std::chrono::steady_clock::now();

        AllocTimeTotal += std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - begin1).count();
        Tmp->Pos = Pos;
        Tmp->Material = Material;
        Tmp->Color = Color;
        Tmp->Transparent = Transparent;
        Tmp->VisibilityMask = mask;

        begin1 = std::chrono::steady_clock::now();
        m_Voxels.insert({Pos, Tmp});
        end1 = std::chrono::steady_clock::now();
        InsertTimeTotal += std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - begin1).count();

        m_BlockCount++;
    }

    void CVoxelMesh::RemoveVoxel(const CVector &Pos)
    {
        std::lock_guard<std::recursive_mutex> lock(m_Lock);
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
        std::lock_guard<std::recursive_mutex> lock(m_Lock);
        m_Voxels.clear();
        //TODO: Fix for V-Edit
        // m_Voxels.clear();

        // if(m_RemeshAll)
        //     InsertMarkedChunk(m_BBox);
        // else
        // {
        //     for (auto &&c : m_Chunks)
        //         InsertMarkedChunk(c.second);
        // }
    }

    Voxel CVoxelMesh::GetVoxel(const CVector &Pos)
    {
        std::lock_guard<std::recursive_mutex> lock(m_Lock);

        std::chrono::steady_clock::time_point begin1 = std::chrono::steady_clock::now();
        auto IT = m_Voxels.find(Pos);
        std::chrono::steady_clock::time_point end1 = std::chrono::steady_clock::now();
        SearchTimeTotal += std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - begin1).count();

        if(IT == m_Voxels.end())
            return nullptr;

        return IT->second;
    }

    Voxel CVoxelMesh::GetVoxel(const CVector &Pos, bool OpaqueOnly)
    {
        auto voxel = GetVoxel(Pos);
        if(voxel)
        {
            if(OpaqueOnly && voxel->Transparent)
                voxel = nullptr;
            else if(!OpaqueOnly && !voxel->Transparent)
                voxel = nullptr;
        }

        return voxel;
    }

    void CVoxelMesh::SetNormal(const CVector &Pos, const CVector &Neighbor, bool IsInvisible)
    {
        static const std::map<CVector, std::pair<CVoxel::Visibility, CVoxel::Visibility>> NEIGHBOR_INDEX = {
            {CVoxel::FACE_UP, {CVoxel::Visibility::UP, CVoxel::Visibility::DOWN}},
            {CVoxel::FACE_DOWN, {CVoxel::Visibility::DOWN, CVoxel::Visibility::UP}},

            {CVoxel::FACE_LEFT, {CVoxel::Visibility::LEFT, CVoxel::Visibility::RIGHT}},
            {CVoxel::FACE_RIGHT, {CVoxel::Visibility::RIGHT, CVoxel::Visibility::LEFT}},

            {CVoxel::FACE_FORWARD, {CVoxel::Visibility::FORWARD, CVoxel::Visibility::BACKWARD}},
            {CVoxel::FACE_BACKWARD, {CVoxel::Visibility::BACKWARD, CVoxel::Visibility::FORWARD}},
        };

        Voxel cur = GetVoxel(Pos);
        if(cur == nullptr)
            return;

        Voxel neighbor = GetVoxel(Pos + Neighbor);
        auto visibility = NEIGHBOR_INDEX.at(Neighbor);

        // 1. Both opaque touching faces invisible
        // 2. One transparent touching faces visible
        // 3. Both transparent different transparency faces visible
        // 4. Both transparent same transparency and color faces invisible
        // 5. Both transparent different transparency and same color faces visible

        if(neighbor)
        {
            bool hideFaces = false;

            if(!neighbor->Transparent && !cur->Transparent)
                hideFaces = true;
            else if(neighbor->Transparent && !cur->Transparent || !neighbor->Transparent && cur->Transparent)
                hideFaces = false;
            else if(neighbor->Transparent && cur->Transparent)
            {
                if(neighbor->Material != cur->Material)
                    hideFaces = false;
                else if(neighbor->Color != cur->Color)
                    hideFaces = false;
                else
                    hideFaces = true;
            }

            if(hideFaces)
            {
                neighbor->VisibilityMask = IsInvisible ? (CVoxel::Visibility)(neighbor->VisibilityMask & ~visibility.second) : (CVoxel::Visibility)(neighbor->VisibilityMask | visibility.second);
                cur->VisibilityMask = IsInvisible ? (CVoxel::Visibility)(cur->VisibilityMask & ~visibility.first) : (CVoxel::Visibility)(cur->VisibilityMask | visibility.first);
            }
            else
                cur->VisibilityMask = (CVoxel::Visibility)(cur->VisibilityMask | visibility.first);
        }
        else
            cur->VisibilityMask = (CVoxel::Visibility)(cur->VisibilityMask | visibility.first);

        // CheckInvisible(cur);
        // if(neighbor)
        //     CheckInvisible(neighbor);
    }
} // namespace VoxelOptimizer
