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
#include <VCore/Voxel/VoxelModel.hpp>

namespace VCore
{
    void CVoxelModel::SetVoxel(const Math::Vec3i &Pos, uint8_t Material, uint32_t Color)
    {      
        CVoxel Tmp;
        
        Tmp.Material = Material;
        Tmp.Color = Color;

        m_Voxels.insert({Pos, Tmp});
    }

    void CVoxelModel::RemoveVoxel(const Math::Vec3i &Pos)
    {
        auto IT = m_Voxels.find(Pos);
        if(IT != m_Voxels.end())
            m_Voxels.erase(IT);
    }
    
    void CVoxelModel::Clear()
    {
        m_Voxels.clear();
    }

    Voxel CVoxelModel::GetVoxel(const Math::Vec3i &Pos)
    {
        auto it = m_Voxels.find(Pos);
        if(it == m_Voxels.end())
            return nullptr;

        return it->second;
    }

    CVoxelModel::VoxelData::querylist CVoxelModel::QueryDirtyChunks()
    {
        return m_Voxels.queryDirtyChunks();
    }

    CVoxelModel::VoxelData::querylist CVoxelModel::QueryChunks() const
    {
        return m_Voxels.queryChunks();
    }

    CVoxelModel::VoxelData::querylist CVoxelModel::QueryChunks(const CFrustum *_Frustum) const
    {
        return m_Voxels.queryChunks(_Frustum);
    }
}
