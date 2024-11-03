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

namespace VCore
{
    //////////////////////////////////////////////////
    // CBitMaskChunk functions
    //////////////////////////////////////////////////

    CBitMaskChunk::CBitMaskChunk(const Math::Vec3i &_ChunkSize)
    {
        if(_ChunkSize != Math::Vec3i::ZERO)
            m_Grid.resize(_ChunkSize.x * _ChunkSize.y * 3, 0);
    }

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
    // CVoxelSpace::CChunk functions
    //////////////////////////////////////////////////

    CChunk::CChunk(const Math::Vec3i &_ChunkSize) : IsDirty(false), m_InnerBBox(Math::Vec3i(INT32_MAX, INT32_MAX, INT32_MAX), Math::Vec3i()), m_Mask(_ChunkSize)
    {
        m_Data = new CVoxel[_ChunkSize.x * _ChunkSize.y * _ChunkSize.z];   
    }

    CChunk::CChunk(CChunk &&_Other) : m_Data(nullptr), m_Mask(Math::Vec3i())
    {
        *this = std::move(_Other);
    }

    void CChunk::insert(CVoxelSpace *_Space, const pair &_pair)
    {
        Math::Vec3i relPos = _pair.first & Config::InnerChunkMask;
        CVoxel &voxel = m_Data[relPos.x + Config::ChunkSize * relPos.y + Config::ChunkSize * Config::ChunkSize * relPos.z];
        voxel.Color = _pair.second.Color;
        voxel.Material = _pair.second.Material;

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
    }

    bool CChunk::HasVoxelOnPlane(int _Axis, const Math::Vec3i &_Pos)
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
                if(m_Data[pos.x + Config::ChunkSize * pos.y + Config::ChunkSize * Config::ChunkSize * pos.z].IsInstantiated())
                    return true;
            }
        }
        
        return false;
    }

    CVoxelSpace::ppair CChunk::erase(CVoxelSpace *_Space, const iterator &_it)
    {
        Math::Vec3i relPos = _it->first & Config::InnerChunkMask;
        CVoxel &voxel = m_Data[relPos.x + Config::ChunkSize * relPos.y + Config::ChunkSize * Config::ChunkSize * relPos.z];
        voxel = CVoxel();
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

    CVoxelSpace::ppair CChunk::next(const Math::Vec3i &_Position) const
    {
        Math::Vec3i relPos = _Position & Config::InnerChunkMask;

        for (int z = relPos.z; z <= m_InnerBBox.End.z; z++)
        {
            for (int y = relPos.y; y <= m_InnerBBox.End.y; y++)
            {
                for (int x = relPos.x; x <= m_InnerBBox.End.x; x++)
                {
                    CVoxel &vox = m_Data[x + Config::ChunkSize * y + Config::ChunkSize * Config::ChunkSize * z];
                    if(vox.IsInstantiated())
                        return {GetChunkpos(_Position) + Math::Vec3i(x, y, z), &vox};
                }

                relPos.x = m_InnerBBox.Beg.x;
            }

            relPos.y = m_InnerBBox.Beg.y;
        }
        
        return {Math::Vec3i(), nullptr};
    }

    Voxel CChunk::find(const Math::Vec3i &_v) const
    {
        Math::Vec3i relPos = _v & Config::InnerChunkMask;
        CVoxel &vox = m_Data[relPos.x + Config::ChunkSize * relPos.y + Config::ChunkSize * Config::ChunkSize * relPos.z];
        if(vox.IsInstantiated())
            return &vox;

        return nullptr;
    }

    void CChunk::clear()
    {
        if(m_Data)
        {
            delete[] m_Data;
            m_Data = nullptr;
        }

        m_InnerBBox = CBBox();
    }

    CChunk &CChunk::operator=(CChunk &&_Other)
    {
        clear();
        m_InnerBBox = _Other.m_InnerBBox;
        m_Data = _Other.m_Data;
        IsDirty = _Other.IsDirty;
        m_Mask = std::move(_Other.m_Mask);

        _Other.m_Data = nullptr;
        _Other.m_InnerBBox = CBBox();
        _Other.IsDirty = false;

        return *this;
    }
} // namespace VCore
