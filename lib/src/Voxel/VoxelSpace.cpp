/*
 * MIT License
 *
 * Copyright (c) 2023 Christian Tost
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

#include <VoxelOptimizer/Voxel/VoxelSpace.hpp>
#include <VoxelOptimizer/Voxel/VoxelMesh.hpp>

namespace VoxelOptimizer
{
    //////////////////////////////////////////////////
    // CVoxelSpace functions
    //////////////////////////////////////////////////

    CVoxelSpace::CVoxelSpace() : m_VoxelsCount(0), m_ChunkSize(16, 16, 16) {}
    CVoxelSpace::CVoxelSpace(const CVectori &_ChunkSize) : CVoxelSpace()
    {
        m_ChunkSize = _ChunkSize;
    }

    CVoxelSpace::CVoxelSpace(CVoxelSpace &&_Other) 
    {
        *this = std::move(_Other);
    }

    void CVoxelSpace::insert(const pair &_pair)
    {
        CVectori position = chunkpos(_pair.first);
        auto it = m_Chunks.find(position);

        // Creates a new chunk, if neccessary
        if(it == m_Chunks.end())
            it = m_Chunks.insert({position, CChunk(m_ChunkSize)}).first;

        it->second.insert(_pair, CBBox(position, m_ChunkSize - CVectori(1, 1, 1)));
        m_VoxelsCount++;
    }

    CVoxelSpace::iterator CVoxelSpace::erase(const iterator &_it)
    {
        CVectori position = chunkpos(_it->first);
        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return end();

        auto res = it->second.erase(_it, CBBox(position, m_ChunkSize - CVectori(1, 1, 1)));
        m_VoxelsCount--;

        // Searches for the next voxel inside of any chunk.
        while (!res.second)
        {
            it++;
            if(it == m_Chunks.end())
                return end();

            res = it->second.next(it->second.inner_bbox(it->first).Beg, CBBox(it->first, m_ChunkSize - CVectori(1, 1, 1)));
        }
        
        if(res.second)
            return CVoxelSpaceIterator(this, it->second.inner_bbox(it->first), res);

        return end();
    }

    CVoxelSpace::iterator CVoxelSpace::find(const CVectori &_v) const
    {
        CVectori position = chunkpos(_v);
        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return end();

        CVoxel *vox = it->second.find(_v, CBBox(position, m_ChunkSize - CVectori(1, 1, 1)));
        if(!vox)
            return end();

        return CVoxelSpaceIterator(this, it->second.inner_bbox(it->first), {_v, vox});
    }

    std::map<CVectori, Voxel> CVoxelSpace::queryVisible(bool opaque) const
    {
        std::map<CVectori, Voxel> ret;
        for (auto &&c : m_Chunks)
        {
            auto bbox = CBBox(c.first, m_ChunkSize - CVectori(1, 1, 1));
            auto inner = c.second.inner_bbox(bbox.Beg);
            for (int z = inner.Beg.z; z < inner.End.z; z++)
            {
                for (int y = inner.Beg.y; y < inner.End.y; y++)
                {
                    for (int x = inner.Beg.x; x < inner.End.x; x++)
                    {
                        auto res = c.second.find(CVectori(x, y, z), bbox);
                        if(res && res->IsVisible() && res->Transparent == !opaque)
                            ret.insert({CVectori(x, y, z), res});
                    }
                }
            }     
        }
        return ret;
    }

    std::list<SChunk> CVoxelSpace::queryDirtyChunks()
    {
        std::list<SChunk> ret;
        for (auto &&c : m_Chunks)
        {
            if(c.second.IsDirty)
            {
                c.second.IsDirty = false;
                ret.push_back({(size_t)&c, CBBox(c.first, c.first + m_ChunkSize - CVectori(1, 1, 1)), c.second.inner_bbox(c.first)});
            }
        }
        return ret;
    }

    std::list<SChunk> CVoxelSpace::queryChunks() const
    {
        std::list<SChunk> ret;
        for (auto &&c : m_Chunks)
            ret.push_back({(size_t)&c, CBBox(c.first, c.first + m_ChunkSize - CVectori(1, 1, 1)), c.second.inner_bbox(c.first)});
        return ret;
    }

    void CVoxelSpace::generateVisibilityMask()
    {
        const static std::vector<std::pair<CVector, int>> AXIS_DIRECTIONS = {
            {CVector(1, 0, 0), 0},
            {CVector(0, 1, 0), 1},
            {CVector(0, 0, 1), 2}
        };

        for (auto &&c : m_Chunks)
        {
            if(!c.second.IsDirty)
                continue;

            auto bbox = CBBox(c.first, m_ChunkSize - CVectori(1, 1, 1));
            auto inner = c.second.inner_bbox(bbox.Beg);
            
            CVectori beg = inner.Beg;
            CVectori end = inner.End;

            // for (int z = beg.z; z < end.z - 1; z++)
            // {
            //     for (int y = beg.y; y < end.y - 1; y++)
            //     {
            //         for (int x = beg.x; x < end.x - 1; x++)
            //         {
            //             Voxel current = c.second.find(CVectori(x, y, z), bbox);

            //             for (auto &&axis : AXIS_DIRECTIONS)
            //             {
            //                 CVectori pos(x, y, z);

            //                 // Checks boundary.
            //                 if(current && pos.v[axis.second] == ((bbox.Beg.v[axis.second] + bbox.End.v[axis.second]) - 1))
            //                 {
            //                     pos.v[axis.second] += 1;

            //                     auto ocit = find(pos);
            //                     if(ocit != this->end())
            //                         CheckVisibility(current, ocit->second, axis.second);
            //                 }
            //                 else
            //                 {
            //                     pos += axis.first;
            //                     Voxel other = c.second.find(pos, bbox);
            //                     if(other)
            //                         CheckVisibility(current, other, axis.second);
            //                 }
            //             }
            //         }
            //     }
            // }
            

            for (char axis = 0; axis < 3; axis++)
            {
                int axis1 = (axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
                int axis2 = (axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y
                Voxel current;

                for (int x = beg.v[axis]; x < end.v[axis]; x++)
                {
                    for (int y = beg.v[axis1]; y < end.v[axis1]; y++)
                    {
                        // Reset current
                        CVectori pos;
                        pos.v[axis] = x;
                        pos.v[axis1] = y;
                        pos.v[axis2] = beg.v[axis2];

                        current = c.second.find(pos, bbox);
                        for (int z = beg.v[axis2] + 1; z < end.v[axis2]; z++)
                        {
                            pos.v[axis] = x;
                            pos.v[axis1] = y;
                            pos.v[axis2] = z;

                            auto second = c.second.find(pos, bbox);
                            if(second)
                            {
                                CheckVisibility(current, second, axis2);
                                current = second;
                            }
                            else
                                current = nullptr;
                        }

                        // Checks boundary.
                        if(current && pos.v[axis2] == ((bbox.Beg.v[axis2] + bbox.End.v[axis2])- 1))
                        {
                            pos.v[axis2] += 1;

                            auto ocit = find(pos);
                            if(ocit != this->end())
                                CheckVisibility(current, ocit->second, axis2);
                        }
                    }
                }
            }
        }
    }

    void CVoxelSpace::CheckVisibility(const Voxel &_v, const Voxel &_v2, char _axis)
    {
        const static std::pair<CVoxel::Visibility, CVoxel::Visibility> ADJACENT_FACES[3] = {
            {~CVoxel::Visibility::RIGHT, ~CVoxel::Visibility::LEFT},
            {~CVoxel::Visibility::FORWARD, ~CVoxel::Visibility::BACKWARD},
            {~CVoxel::Visibility::UP, ~CVoxel::Visibility::DOWN},
        };

        if(_v)
        {
            // 1. Both opaque touching faces invisible
            // 2. One transparent touching faces visible
            // 3. Both transparent different transparency faces visible
            // 4. Both transparent same transparency and color faces invisible
            // 5. Both transparent different transparency and same color faces visible

            bool hideFaces = false;
            if(!_v2->Transparent && !_v->Transparent)
                hideFaces = true;
            else if(_v2->Transparent && !_v->Transparent || !_v2->Transparent && _v->Transparent)
                hideFaces = false;
            else if(_v2->Transparent && _v->Transparent)
            {
                if(_v2->Material != _v->Material)
                    hideFaces = false;
                else if(_v2->Color != _v->Color)
                    hideFaces = false;
                else
                    hideFaces = true;
            }

            if(hideFaces)
            {
                const std::pair<CVoxel::Visibility, CVoxel::Visibility> &adjacent_faces = ADJACENT_FACES[_axis];

                _v->VisibilityMask &= adjacent_faces.first;
                _v2->VisibilityMask &= adjacent_faces.second;
            }
        }
    }

    CVoxelSpace::iterator CVoxelSpace::next(const CVectori &_FromPosition) const
    {
        CVectori position = chunkpos(_FromPosition);
        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return end();

        auto res = it->second.next(_FromPosition, CBBox(position, m_ChunkSize - CVectori(1, 1, 1)));
        
        // Searches for the next voxel inside of any chunk.
        while (!res.second)
        {
            it++;
            if(it == m_Chunks.end())
                return end();

            res = it->second.next(it->second.inner_bbox(it->first).Beg, CBBox(it->first, m_ChunkSize - CVectori(1, 1, 1)));
        }

        if(res.second)
            return CVoxelSpaceIterator(this, it->second.inner_bbox(it->first), res);

        return end();
    }

    void CVoxelSpace::updateVisibility(const CVectori &_Position)
    {
        auto it = this->find(_Position);
        if(it == this->end())
        {
            const static std::vector<std::pair<CVector, CVoxel::Visibility>> DIRECTIONS = {
                {CVector(1, 0, 0), CVoxel::Visibility::LEFT},
                {CVector(-1, 0, 0), CVoxel::Visibility::RIGHT},
                {CVector(0, 1, 0), CVoxel::Visibility::BACKWARD},
                {CVector(0, -1, 0), CVoxel::Visibility::FORWARD},
                {CVector(0, 0, -1), CVoxel::Visibility::UP},
                {CVector(0, 0, 1), CVoxel::Visibility::DOWN}
            };

            for (auto &&dir : DIRECTIONS)
            {
                it = this->find(_Position + dir.first);
                if(it != this->end())
                    it->second->VisibilityMask |= dir.second;
            }
        }
        else
        {
            const static std::vector<std::pair<CVector, int>> AXIS_DIRECTIONS = {
                {CVector(1, 0, 0), 0},
                {CVector(0, 1, 0), 1},
                {CVector(0, 0, 1), 2}
            };

            for (auto &&axis : AXIS_DIRECTIONS)
            {
                CVector start = _Position - axis.first;
                for (char i = 0; i < 2; i++)
                {
                    it = this->find(start);
                    auto IT2 = this->find(start + axis.first);

                    if(it != this->end() && IT2 != this->end())
                        CheckVisibility(it->second, IT2->second, axis.second);

                    start += axis.first;
                }
            }
        }
    }

    CVoxelSpace::iterator CVoxelSpace::begin()
    {
        if(m_Chunks.empty())
            return end();

        auto it = m_Chunks.begin();
        auto bbox = it->second.inner_bbox(it->first);
        return CVoxelSpaceIterator(this, bbox, it->second.next(bbox.Beg, CBBox(it->first, m_ChunkSize - CVectori(1, 1, 1))));
    }

    CVoxelSpace::iterator CVoxelSpace::end() const
    {
        return CVoxelSpaceIterator(this, CBBox(), {CVectori(), nullptr});
    }

    void CVoxelSpace::clear()
    {
        m_Chunks.clear();
    }

    CVoxelSpace &CVoxelSpace::operator=(CVoxelSpace &&_Other)
    {
        m_Size = _Other.m_Size;
        m_ChunkSize = _Other.m_ChunkSize;
        m_VoxelsCount = _Other.m_VoxelsCount;
        m_Chunks = std::move(_Other.m_Chunks);

        return *this;
    }

    CVectori CVoxelSpace::chunkpos(const CVectori &_Position) const
    {
        return (CVector(_Position) / m_ChunkSize).Floor() * m_ChunkSize;
    }

    //////////////////////////////////////////////////
    // CVoxelSpace::CChunk functions
    //////////////////////////////////////////////////

    CVoxelSpace::CChunk::CChunk(const CVectori &_ChunkSize) : m_InnerBBox(CVectori(INT32_MAX, INT32_MAX, INT32_MAX), CVectori()), IsDirty(false)
    {
        m_Data = new CVoxel[_ChunkSize.x * _ChunkSize.y * _ChunkSize.z];   
    }

    CVoxelSpace::CChunk::CChunk(CChunk &&_Other) : m_Data(nullptr)
    {
        *this = std::move(_Other);
    }

    void CVoxelSpace::CChunk::insert(const pair &_pair, const CBBox &_ChunkDim)
    {
        CVectori relPos = (_pair.first - _ChunkDim.Beg).Abs();
        m_Data[relPos.x + _ChunkDim.End.x * relPos.y + _ChunkDim.End.x * _ChunkDim.End.y * relPos.z] = _pair.second;
    
        m_InnerBBox.Beg = m_InnerBBox.Beg.Min(relPos);
        m_InnerBBox.End = m_InnerBBox.End.Max(relPos);
        IsDirty = true;
    }

    CVoxelSpace::ppair CVoxelSpace::CChunk::erase(const iterator &_it, const CBBox &_ChunkDim)
    {
        CVectori relPos = (_it->first - _ChunkDim.Beg).Abs();
        m_Data[relPos.x + _ChunkDim.End.x * relPos.y + _ChunkDim.End.x * _ChunkDim.End.y * relPos.z] = CVoxel();
        
        return next(_it->first, _ChunkDim);
    }

    CVoxelSpace::ppair CVoxelSpace::CChunk::next(const CVectori &_Position, const CBBox &_ChunkDim) const
    {
        CVectori relPos = (_Position - _ChunkDim.Beg).Abs();
        for (int z = relPos.z; z < m_InnerBBox.End.z; z++)
        {
            for (int y = relPos.y; y < m_InnerBBox.End.y; y++)
            {
                for (int x = relPos.x; x < m_InnerBBox.End.x; x++)
                {
                    CVoxel &vox = m_Data[x + _ChunkDim.End.x * y + _ChunkDim.End.x * _ChunkDim.End.y * z];
                    if(vox.IsVisible())
                        return {_ChunkDim.Beg + CVectori(x, y, z), &vox};
                }
            }
        }
        
        return {CVectori(), nullptr};
    }

    Voxel CVoxelSpace::CChunk::find(const CVectori &_v, const CBBox &_ChunkDim) const
    {
        CVectori relPos = (_v - _ChunkDim.Beg).Abs();

        if(relPos.x + _ChunkDim.End.x * relPos.y + _ChunkDim.End.x * _ChunkDim.End.y * relPos.z > 4096)
        {
            int i = 0;
            i++;
        }

        CVoxel &vox = m_Data[relPos.x + _ChunkDim.End.x * relPos.y + _ChunkDim.End.x * _ChunkDim.End.y * relPos.z];
        if(vox.IsVisible())
            return &vox;

        return nullptr;
    }

    void CVoxelSpace::CChunk::clear()
    {
        if(m_Data)
        {
            delete[] m_Data;
            m_Data = nullptr;
        }

        m_InnerBBox = CBBox();
    }

    CVoxelSpace::CChunk &CVoxelSpace::CChunk::operator=(CChunk &&_Other)
    {
        clear();
        m_InnerBBox = _Other.m_InnerBBox;
        m_Data = _Other.m_Data;
        IsDirty = _Other.IsDirty;

        _Other.m_Data = nullptr;
        _Other.m_InnerBBox = CBBox();
        _Other.IsDirty = false;

        return *this;
    }

    //////////////////////////////////////////////////
    // CVoxelSpaceIterator functions
    //////////////////////////////////////////////////

    CVoxelSpaceIterator::CVoxelSpaceIterator() : m_Space(nullptr), m_Pair(CVectori(), nullptr) { }
    CVoxelSpaceIterator::CVoxelSpaceIterator(const CVoxelSpace *_Space, const CBBox &_InnerBox, const pair &_Pair) : m_Space(_Space), m_InnerBox(_InnerBox), m_Pair(_Pair) { }
    CVoxelSpaceIterator::CVoxelSpaceIterator(const CVoxelSpaceIterator &_Other)
    {
        *this = _Other;
    }

    CVoxelSpaceIterator::CVoxelSpaceIterator(CVoxelSpaceIterator &&_Other)
    {
        *this = std::move(_Other);
    }

    const typename CVoxelSpaceIterator::reference CVoxelSpaceIterator::operator*() const
    {
        return m_Pair;
    }

    const typename CVoxelSpaceIterator::pointer CVoxelSpaceIterator::operator->() const
    {
        return &m_Pair;
    }
    
    CVoxelSpaceIterator& CVoxelSpaceIterator::operator++()
    {
        *this = m_Space->next(m_Pair.first);
        return *this;
    }

    CVoxelSpaceIterator& CVoxelSpaceIterator::operator++(int)
    {
        *this = m_Space->next(m_Pair.first);
        return *this;
    }

    bool CVoxelSpaceIterator::operator!=(const CVoxelSpaceIterator &_Rhs)
    {
        return (m_Space != _Rhs.m_Space) || (m_Pair.first != _Rhs.m_Pair.first) || (m_Pair.second != _Rhs.m_Pair.second);
    }

    bool CVoxelSpaceIterator::operator==(const CVoxelSpaceIterator &_Rhs)
    {
        return (m_Space == _Rhs.m_Space) && (m_Pair.first == _Rhs.m_Pair.first) && (m_Pair.second == _Rhs.m_Pair.second);
    }

    CVoxelSpaceIterator& CVoxelSpaceIterator::operator=(const CVoxelSpaceIterator &_Other)
    {
        m_Space = _Other.m_Space;
        m_Pair = _Other.m_Pair;
        m_InnerBox = _Other.m_InnerBox;

        return *this;
    }

    CVoxelSpaceIterator& CVoxelSpaceIterator::operator=(CVoxelSpaceIterator &&_Other)
    {
        m_Space = _Other.m_Space;
        m_Pair = _Other.m_Pair;
        m_InnerBox = _Other.m_InnerBox;

        _Other.m_Space = nullptr;
        _Other.m_Pair = {CVectori(), nullptr};
        _Other.m_InnerBox = CBBox();

        return *this;
    }
}