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
#include "../../Misc/Helper.hpp"

namespace VCore
{
    CMemoryPool<CChunk> CChunk::m_Pool;
    CMemoryPool<CByteChunk> CByteChunk::m_Pool;

    //////////////////////////////////////////////////
    // CBitMaskChunk functions
    //////////////////////////////////////////////////

    void CBitMaskChunk::SetAxis(const Math::Vec3i &_Position, bool _Value, char _Axis)
    {
        if(_Value)
        {
            switch (_Axis)
            {
                case 0: m_Grid[(_Position.z) + Config::ChunkSize * (_Position.y)] |= ((Config::bitmask_t)1 << (_Position.x + 1)); break;
                case 1: m_Grid[(_Position.x) + Config::ChunkSize * (_Position.z) + (Config::ChunkSize * Config::ChunkSize)] |= ((Config::bitmask_t)1 << (_Position.y + 1)); break;
                case 2: m_Grid[(_Position.x) + Config::ChunkSize * (_Position.y) + (Config::ChunkSize * Config::ChunkSize * 2)] |= ((Config::bitmask_t)1 << (_Position.z + 1)); break;
            }
        }
        else
        {
            switch (_Axis)
            {
                case 0: m_Grid[(_Position.z) + Config::ChunkSize * (_Position.y)] &= ~((Config::bitmask_t)1 << (_Position.x + 1)); break;
                case 1: m_Grid[(_Position.x) + Config::ChunkSize * (_Position.z) + (Config::ChunkSize * Config::ChunkSize)] &= ~((Config::bitmask_t)1 << (_Position.y + 1)); break;
                case 2: m_Grid[(_Position.x) + Config::ChunkSize * (_Position.y) + (Config::ChunkSize * Config::ChunkSize * 2)] &= ~((Config::bitmask_t)1 << (_Position.z + 1)); break;
            }
        }
    }

    CBitMaskChunk &CBitMaskChunk::operator=(const CBitMaskChunk &_Other)
    {
        memcpy(m_Grid, _Other.m_Grid, sizeof(m_Grid));
        return *this;
    }

    void CBitMaskChunk::Set(const Math::Vec3i &_Position, bool _Value)
    {
        if(_Value)
        {
            m_Grid[(_Position.z) + Config::ChunkSize * (_Position.y)] |= ((Config::bitmask_t)1 << (_Position.x + 1));
            m_Grid[(_Position.x) + Config::ChunkSize * (_Position.z) + (Config::ChunkSize * Config::ChunkSize)] |= ((Config::bitmask_t)1 << (_Position.y + 1));
            m_Grid[(_Position.x) + Config::ChunkSize * (_Position.y) + (Config::ChunkSize * Config::ChunkSize * 2)] |= ((Config::bitmask_t)1 << (_Position.z + 1));
        }
        else
        {
            m_Grid[(_Position.z) + Config::ChunkSize * (_Position.y)] &= ~((Config::bitmask_t)1 << (_Position.x + 1));
            m_Grid[(_Position.x) + Config::ChunkSize * (_Position.z) + (Config::ChunkSize * Config::ChunkSize)] &= ~((Config::bitmask_t)1 << (_Position.y + 1));
            m_Grid[(_Position.x) + Config::ChunkSize * (_Position.y) + (Config::ChunkSize * Config::ChunkSize * 2)] &= ~((Config::bitmask_t)1 << (_Position.z + 1));
        }
    }

    Config::bitmask_t CBitMaskChunk::GetRowFaces(const Math::Vec3i &_Position, char _Axis) const
    {
        switch (_Axis)
        {
            case 0: return m_Grid[(_Position.z) + Config::ChunkSize * (_Position.y)];
            case 1: return m_Grid[(_Position.x) + Config::ChunkSize * (_Position.z) + (Config::ChunkSize * Config::ChunkSize)];
            case 2: return m_Grid[(_Position.x) + Config::ChunkSize * (_Position.y) + (Config::ChunkSize * Config::ChunkSize * 2)];
        }

        return 0;
    }

    //////////////////////////////////////////////////
    // IChunk functions
    //////////////////////////////////////////////////

    IChunk *IChunk::Upgrade()
    {
        IChunk *result = nullptr;
        if(dynamic_cast<CChunk*>(this) == nullptr)
        {
            result = new CChunk();
            result->m_InnerBBox = m_InnerBBox;
            for (int x = m_InnerBBox.Beg.x; x < m_InnerBBox.End.x; x++)
            {
                for (int y = m_InnerBBox.Beg.y; y < m_InnerBBox.End.y; y++)
                {
                    for (int z = m_InnerBBox.Beg.z; z < m_InnerBBox.End.z; z++)
                    {
                        Math::Vec3i position(x, y, z);
                        result->SetVoxel(GetVoxel(position), position);
                    }
                }
            } 
        }

        return result;
    }

    bool IChunk::insert(CVoxelSpace *_Space, const pair &_pair)
    {
        Math::Vec3i relPos = _pair.first & Config::InnerChunkMask;
        auto result = SetVoxel(_pair.second, relPos);

        // CVoxel &voxel = m_Data[relPos.x + Config::ChunkSize * relPos.y + Config::ChunkSize * Config::ChunkSize * relPos.z];
        // voxel.Color = _pair.second.Color;
        // voxel.Material = _pair.second.Material;

        m_Mask.Set(relPos, true);

        for (size_t i = 0; i < 3; i++)
        {
            if(relPos.v[i] == Config::InnerChunkMask)
            {
                auto globalPos = _pair.first;
                globalPos.v[i]++;
                auto chunk = _Space->GetChunk(globalPos);
                if(chunk)
                {
                    chunk->IsDirty = true;
                    auto chunkpos = GetChunkpos(globalPos);

                    auto faces = chunk->m_Mask.GetRowFaces(globalPos - chunkpos, i);
                    if(faces & 0x2)
                    {
                        auto tmp = relPos;
                        tmp.v[i]++;
                        m_Mask.SetAxis(tmp, true, i);
                    }

                    globalPos.v[i]--;
                    chunk->m_Mask.SetAxis(globalPos - chunkpos, true, i);
                }
            }
            else if(relPos.v[i] == 0)
            {
                auto globalPos = _pair.first;
                globalPos.v[i]--;
                auto chunk = _Space->GetChunk(globalPos);
                if(chunk == this)
                {
                    int i = 0;
                    i++;
                }

                if(chunk)
                {
                    chunk->IsDirty = true;
                    auto chunkpos = GetChunkpos(globalPos);

                    auto faces = chunk->m_Mask.GetRowFaces(globalPos - chunkpos, i);
                    if(faces & (Config::FaceMask + 1))
                    {
                        auto tmp = relPos;
                        tmp.v[i]--;
                        m_Mask.SetAxis(tmp, true, i);
                    }

                    globalPos.v[i]++;
                    chunk->m_Mask.SetAxis(globalPos - chunkpos, true, i);
                }
            }
        }

        m_InnerBBox.Beg = m_InnerBBox.Beg.min(relPos);
        m_InnerBBox.End = m_InnerBBox.End.max(relPos);
        IsDirty = true;

        return result;
    }

    bool IChunk::HasVoxelOnPlane(int _Axis, const Math::Vec3i &_Pos)
    {
        Math::Vec3i pos = _Pos;
        int heightAxis = (_Axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
        int widthAxis = (_Axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y

        pos.v[widthAxis] = m_InnerBBox.Beg.v[widthAxis];
        for (; pos.v[widthAxis] <= m_InnerBBox.End.v[widthAxis]; pos.v[widthAxis]++)
        {
            pos.v[heightAxis] = m_InnerBBox.Beg.v[heightAxis];
            for (; pos.v[heightAxis] <= m_InnerBBox.End.v[heightAxis]; pos.v[heightAxis]++)
            {
                // if(m_Data[pos.x + Config::ChunkSize * pos.y + Config::ChunkSize * Config::ChunkSize * pos.z].IsInstantiated())
                if(GetVoxel(pos).IsInstantiated())
                    return true;
            }
        }
        
        return false;
    }

    IChunk::pair IChunk::erase(CVoxelSpace *_Space, const iterator &_it)
    {
        Math::Vec3i relPos = _it->first & Config::InnerChunkMask;
        SetVoxel(CVoxel(), relPos);
        // CVoxel &voxel = m_Data[relPos.x + Config::ChunkSize * relPos.y + Config::ChunkSize * Config::ChunkSize * relPos.z];
        // voxel = CVoxel();
        IsDirty = true;

        m_Mask.Set(relPos, false);

        // Checks if the bbox must be resized
        for (size_t i = 0; i < 3; i++)
        {
            if((relPos.v[i] == m_InnerBBox.End.v[i]) && !HasVoxelOnPlane(i, relPos))
                m_InnerBBox.End.v[i] -= 1;
            else if((relPos.v[i] == m_InnerBBox.Beg.v[i]) && !HasVoxelOnPlane(i, relPos))
                m_InnerBBox.Beg.v[i] += 1;
        }
        
        return next(_it->first);
    }

    IChunk::pair IChunk::next(const Math::Vec3i &_Position) const
    {
        Math::Vec3i relPos = _Position & Config::InnerChunkMask;

        for (int z = relPos.z; z <= m_InnerBBox.End.z; z++)
        {
            for (int y = relPos.y; y <= m_InnerBBox.End.y; y++)
            {
                for (int x = relPos.x; x <= m_InnerBBox.End.x; x++)
                {
                    CVoxel vox = GetVoxel(Math::Vec3i(x, y, z)); //m_Data[x + Config::ChunkSize * y + Config::ChunkSize * Config::ChunkSize * z];
                    if(vox.IsInstantiated())
                        return {GetChunkpos(_Position) + Math::Vec3i(x, y, z), vox};
                }

                relPos.x = m_InnerBBox.Beg.x;
            }

            relPos.y = m_InnerBBox.Beg.y;
        }
        
        return {Math::Vec3i(), CVoxel()};
    }

    CVoxel IChunk::find(const Math::Vec3i &_v) const
    {
        Math::Vec3i relPos = _v & Config::InnerChunkMask;
        // CVoxel vox = m_Data[relPos.x + Config::ChunkSize * relPos.y + Config::ChunkSize * Config::ChunkSize * relPos.z];
        return GetVoxel(relPos);
    }

    //////////////////////////////////////////////////
    // CByteChunk functions
    //////////////////////////////////////////////////

    uint8_t CByteChunk::AddAndGetVoxelIndex(const CVoxel &_Voxel)
    {
        int index = (uint32_t)_Voxel % HASHMAP_SIZE;

        while (m_VoxelIndex[index].RefCount != 0 && m_VoxelIndex[index].Voxel != _Voxel)
            index = (index + 1) % HASHMAP_SIZE;
        
        if(m_VoxelIndex[index].RefCount == 0)
            m_VoxelIndexSize++;

        m_VoxelIndex[index].Voxel = _Voxel;
        m_VoxelIndex[index].RefCount++;

        return index;
    }

    void CByteChunk::Clear()
    {
        memset(m_Data, 0xFF, sizeof(m_Data));
        m_InnerBBox = CBBox();
    }

    bool CByteChunk::SetVoxel(const CVoxel &_Voxel, const Math::Vec3i &_Position)
    {
        auto voxelIndex = _Position.x + Config::ChunkSize * _Position.y + Config::ChunkSize * Config::ChunkSize * _Position.z;
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

        if(!_Voxel.IsInstantiated())
        {
            m_Data[voxelIndex] = 0xFF;
            return true;
        }

        // Upgrade this chunk, because there are no more slots for voxels.
        if(m_VoxelIndexSize == HASHMAP_SIZE)
            return false;

        // Adds the index of the hashmap
        m_Data[voxelIndex] = AddAndGetVoxelIndex(_Voxel);
        return true;
    }

    CVoxel CByteChunk::GetVoxel(const Math::Vec3i &_Position) const
    {
        auto voxelIndex = _Position.x + Config::ChunkSize * _Position.y + Config::ChunkSize * Config::ChunkSize * _Position.z;
        auto voxel = m_Data[voxelIndex];

        if(voxel != 0xFF)
            return m_VoxelIndex[voxel].Voxel;

        return CVoxel();
    }

    //////////////////////////////////////////////////
    // CChunk functions
    //////////////////////////////////////////////////

    bool CChunk::SetVoxel(const CVoxel &_Voxel, const Math::Vec3i &_Position)
    {
        m_Data[_Position.x + Config::ChunkSize * _Position.y + Config::ChunkSize * Config::ChunkSize * _Position.z] = _Voxel;

        return true;
    }

    CVoxel CChunk::GetVoxel(const Math::Vec3i &_Position) const
    {
        return m_Data[_Position.x + Config::ChunkSize * _Position.y + Config::ChunkSize * Config::ChunkSize * _Position.z];
    }

    void CChunk::Clear()
    {
        memset(m_Data, 0xFFFFFFFF, sizeof(m_Data));
        m_InnerBBox = CBBox();
    }
} // namespace VCore
