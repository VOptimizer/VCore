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

#include <VCore/Voxel/Storage/Chunk.hpp>
#include <VCore/Voxel/Storage/VoxelSpace.hpp>
#include <cstddef>
#include <VCore/Debug.hpp>

namespace VCore
{
    //////////////////////////////////////////////////
    // CBitMaskChunk functions
    //////////////////////////////////////////////////

    void CBitMaskChunk::SetAxis(const Math::Vec3i &p_Position, bool p_Value, char p_Axis)
    {
        if(p_Value)
        {
            switch (p_Axis)
            {
                case 0: m_Grid[(p_Position.z) + Config::ChunkSize * (p_Position.y)] |= ((Config::bitmask_t)1 << (p_Position.x)); break;
                case 1: m_Grid[(p_Position.x) + Config::ChunkSize * (p_Position.z) + (Config::ChunkSize * Config::ChunkSize)] |= ((Config::bitmask_t)1 << (p_Position.y)); break;
                case 2: m_Grid[(p_Position.x) + Config::ChunkSize * (p_Position.y) + (Config::ChunkSize * Config::ChunkSize * 2)] |= ((Config::bitmask_t)1 << (p_Position.z)); break;
            }
        }
        else
        {
            switch (p_Axis)
            {
                case 0: m_Grid[(p_Position.z) + Config::ChunkSize * (p_Position.y)] &= ~((Config::bitmask_t)1 << (p_Position.x)); break;
                case 1: m_Grid[(p_Position.x) + Config::ChunkSize * (p_Position.z) + (Config::ChunkSize * Config::ChunkSize)] &= ~((Config::bitmask_t)1 << (p_Position.y)); break;
                case 2: m_Grid[(p_Position.x) + Config::ChunkSize * (p_Position.y) + (Config::ChunkSize * Config::ChunkSize * 2)] &= ~((Config::bitmask_t)1 << (p_Position.z)); break;
            }
        }
    }

    CBitMaskChunk &CBitMaskChunk::operator=(const CBitMaskChunk &p_Other)
    {
        memcpy(m_Grid, p_Other.m_Grid, sizeof(m_Grid));
        return *this;
    }

    void CBitMaskChunk::Set(const Math::Vec3i &p_Position, bool p_Value)
    {
        if(p_Value)
        {
            m_Grid[(p_Position.z) + Config::ChunkSize * (p_Position.y)] |= ((Config::bitmask_t)1 << (p_Position.x));
            m_Grid[(p_Position.x) + Config::ChunkSize * (p_Position.z) + (Config::ChunkSize * Config::ChunkSize)] |= ((Config::bitmask_t)1 << (p_Position.y));
            m_Grid[(p_Position.x) + Config::ChunkSize * (p_Position.y) + (Config::ChunkSize * Config::ChunkSize * 2)] |= ((Config::bitmask_t)1 << (p_Position.z));
        }
        else
        {
            m_Grid[(p_Position.z) + Config::ChunkSize * (p_Position.y)] &= ~((Config::bitmask_t)1 << (p_Position.x));
            m_Grid[(p_Position.x) + Config::ChunkSize * (p_Position.z) + (Config::ChunkSize * Config::ChunkSize)] &= ~((Config::bitmask_t)1 << (p_Position.y));
            m_Grid[(p_Position.x) + Config::ChunkSize * (p_Position.y) + (Config::ChunkSize * Config::ChunkSize * 2)] &= ~((Config::bitmask_t)1 << (p_Position.z));
        }
    }

    Config::bitmask_t CBitMaskChunk::GetRowFaces(const Math::Vec3i &p_Position, char p_Axis) const
    {
        switch (p_Axis)
        {
            case 0: return m_Grid[(p_Position.z) + Config::ChunkSize * (p_Position.y)];
            case 1: return m_Grid[(p_Position.x) + Config::ChunkSize * (p_Position.z) + (Config::ChunkSize * Config::ChunkSize)];
            case 2: return m_Grid[(p_Position.x) + Config::ChunkSize * (p_Position.y) + (Config::ChunkSize * Config::ChunkSize * 2)];
        }

        return 0;
    }

    //////////////////////////////////////////////////
    // IChunk functions
    //////////////////////////////////////////////////

    void CChunk::Upgrade()
    {
        if(m_Storage && dynamic_cast<CChunk32*>(m_Storage) == nullptr)
        {
            CChunk32 *result = new CChunk32();
            for (int x = m_InnerBBox.Beg.x; x <= m_InnerBBox.End.x; x++)
            {
                for (int y = m_InnerBBox.Beg.y; y <= m_InnerBBox.End.y; y++)
                {
                    for (int z = m_InnerBBox.Beg.z; z <= m_InnerBBox.End.z; z++)
                    {
                        Math::Vec3i position(x, y, z);
                        result->SetVoxel(m_Storage->GetVoxel(position), position);
                    }
                }
            }

            delete m_Storage;
            m_Storage = result;
        }
    }

    void CChunk::UpdateNeighborChunks(CVoxelSpace *p_Space, const Math::Vec3i &p_GlobalPos)
    {
        Math::Vec3i relPos = p_GlobalPos & Config::InnerChunkMask;
        for (size_t i = 0; i < 3; i++)
        {
            if(relPos.v[i] == Config::InnerChunkMask)
            {
                auto globalPos = p_GlobalPos;
                globalPos.v[i]++;
                auto chunk = p_Space->GetChunk(globalPos);
                if(chunk)
                    chunk->IsDirty = true;
            }
            else if(relPos.v[i] == 0)
            {
                auto globalPos = p_GlobalPos;
                globalPos.v[i]--;
                auto chunk = p_Space->GetChunk(globalPos);
                if(chunk)
                    chunk->IsDirty = true;
            }
        }
    }

    bool CChunk::insert(const pair &p_pair)
    {
        Math::Vec3i relPos = p_pair.first & Config::InnerChunkMask;
        if(!m_Storage)
            m_Storage = new CByteChunk();

        bool result = m_Storage->SetVoxel(p_pair.second, relPos);
        Mask.Set(relPos, true);
        UpdateNeighborChunks(m_Space, p_pair.first);

        m_InnerBBox.Beg = m_InnerBBox.Beg.min(relPos);
        m_InnerBBox.End = m_InnerBBox.End.max(relPos);
        IsDirty = true;

        return result;
    }

    bool CChunk::HasVoxelOnPlane(int p_Axis, const Math::Vec3i &p_Pos)
    {
        Math::Vec3i pos = p_Pos;
        int heightAxis = (p_Axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
        int widthAxis = (p_Axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y

        pos.v[widthAxis] = m_InnerBBox.Beg.v[widthAxis];
        for (; pos.v[widthAxis] <= m_InnerBBox.End.v[widthAxis]; pos.v[widthAxis]++)
        {
            pos.v[heightAxis] = m_InnerBBox.Beg.v[heightAxis];
            for (; pos.v[heightAxis] <= m_InnerBBox.End.v[heightAxis]; pos.v[heightAxis]++)
            {
                if(m_Storage->GetVoxel(pos).IsInstantiated())
                    return true;
            }
        }
        
        return false;
    }

    CChunk::pair CChunk::erase(const iterator &p_it)
    {
        if(!m_Storage)
            return { Math::Vec3i(), CVoxel() };

        Math::Vec3i relPos = p_it->first & Config::InnerChunkMask;
        m_Storage->SetVoxel(CVoxel(), relPos);
        IsDirty = true;

        Mask.Set(relPos, false);
        UpdateNeighborChunks(m_Space, p_it->first);

        // Checks if the bbox must be resized
        for (size_t i = 0; i < 3; i++)
        {
            if((relPos.v[i] == m_InnerBBox.End.v[i]) && !HasVoxelOnPlane(i, relPos))
                m_InnerBBox.End.v[i] -= 1;
            else if((relPos.v[i] == m_InnerBBox.Beg.v[i]) && !HasVoxelOnPlane(i, relPos))
                m_InnerBBox.Beg.v[i] += 1;
        }

        // Check if there any remaining voxels.
        if(
            m_InnerBBox.Beg.x > m_InnerBBox.End.x && 
            m_InnerBBox.Beg.y > m_InnerBBox.End.y && 
            m_InnerBBox.Beg.z > m_InnerBBox.End.z
        )
        {
            delete m_Storage;
            m_Storage = nullptr;
            return { Math::Vec3i(), CVoxel() };
        }
        
        return next(p_it->first);
    }

    CChunk::pair CChunk::next(const Math::Vec3i &p_Position) const
    {
        Math::Vec3i relPos = p_Position & Config::InnerChunkMask;

        for (int z = relPos.z; z <= m_InnerBBox.End.z; z++)
        {
            for (int y = relPos.y; y <= m_InnerBBox.End.y; y++)
            {
                for (int x = relPos.x; x <= m_InnerBBox.End.x; x++)
                {
                    CVoxel vox = m_Storage->GetVoxel(Math::Vec3i(x, y, z)); //m_Data[x + Config::ChunkSize * y + Config::ChunkSize * Config::ChunkSize * z];
                    if(vox.IsInstantiated())
                        return {GetChunkpos(p_Position) + Math::Vec3i(x, y, z), vox};
                }

                relPos.x = m_InnerBBox.Beg.x;
            }

            relPos.y = m_InnerBBox.Beg.y;
        }
        
        return {Math::Vec3i(), CVoxel()};
    }

    CVoxel CChunk::find(const Math::Vec3i &p_v) const
    {
        if(!m_Storage)
            return CVoxel();

        Math::Vec3i relPos = p_v & Config::InnerChunkMask;
        return m_Storage->GetVoxel(relPos);
    }

    bool CChunk::HasVoxel(const Math::Vec3i &p_v) const
    {
        if(!m_Storage)
            return false;

        Math::Vec3i relPos = p_v & Config::InnerChunkMask;
        return (Mask.GetRowFaces(relPos, 1) >> relPos.y) & 1;
    }

    //////////////////////////////////////////////////
    // CByteChunk functions
    //////////////////////////////////////////////////

    uint8_t CByteChunk::AddAndGetVoxelIndex(const CVoxel &p_Voxel)
    {
        int index = (uint32_t)p_Voxel % HASHMAP_SIZE;

        // Guard is inside the SetVoxel method.
        while (m_VoxelIndex[index].RefCount != 0 && m_VoxelIndex[index].Voxel != p_Voxel)
            index = (index + 1) % HASHMAP_SIZE;
        
        auto &voxelIndex = m_VoxelIndex[index];
        if(voxelIndex.RefCount == 0)
            m_VoxelIndexSize++;

        voxelIndex.Voxel = p_Voxel;
        voxelIndex.RefCount++;

        return index;
    }

    void CByteChunk::Clear()
    {
        memset(m_Data, 0xFF, sizeof(m_Data));
    }

    bool CByteChunk::SetVoxel(const CVoxel &p_Voxel, const Math::Vec3i &p_Position)
    {
        const auto voxelIndex = p_Position.x + Config::ChunkSize * p_Position.y + Config::ChunkSize * Config::ChunkSize * p_Position.z;
        auto voxel = m_Data[voxelIndex];

        // Decrements the old voxel by one.
        if(voxel != 0xFF)
        {
            m_VoxelIndex[voxel].RefCount--;
            if(m_VoxelIndex[voxel].RefCount == 0)
            {
                m_VoxelIndexSize--;
                m_VoxelIndex[voxel].Voxel = CVoxel();
            }
        }

        // Deletes the voxel inside the grid.
        if(!p_Voxel.IsInstantiated())
        {
            m_Data[voxelIndex] = 0xFF;
            return true;
        }

        // Upgrade this chunk, because there are no more slots for voxels.
        if(m_VoxelIndexSize == HASHMAP_SIZE)
            return false;

        // Adds the index of the hashmap
        m_Data[voxelIndex] = AddAndGetVoxelIndex(p_Voxel);
        return true;
    }

    CVoxel CByteChunk::GetVoxel(const Math::Vec3i &p_Position) const
    {
        auto voxelIndex = p_Position.x + Config::ChunkSize * p_Position.y + Config::ChunkSize * Config::ChunkSize * p_Position.z;
        auto voxel = m_Data[voxelIndex];

        if(voxel != 0xFF)
            return m_VoxelIndex[voxel].Voxel;

        return CVoxel();
    }

    //////////////////////////////////////////////////
    // CChunk32 functions
    //////////////////////////////////////////////////

    bool CChunk32::SetVoxel(const CVoxel &p_Voxel, const Math::Vec3i &p_Position)
    {
        m_Data[p_Position.x + Config::ChunkSize * p_Position.y + Config::ChunkSize * Config::ChunkSize * p_Position.z] = p_Voxel;

        return true;
    }

    CVoxel CChunk32::GetVoxel(const Math::Vec3i &p_Position) const
    {
        return m_Data[p_Position.x + Config::ChunkSize * p_Position.y + Config::ChunkSize * Config::ChunkSize * p_Position.z];
    }

    void CChunk32::Clear()
    {
        memset(reinterpret_cast<char*>(m_Data), 0xFF, sizeof(m_Data));
    }
} // namespace VCore
