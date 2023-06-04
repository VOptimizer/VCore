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
    CVoxelSpace::CVoxelSpace(const Math::Vec3i &_ChunkSize) : CVoxelSpace()
    {
        m_ChunkSize = _ChunkSize;
    }

    CVoxelSpace::CVoxelSpace(CVoxelSpace &&_Other) 
    {
        *this = std::move(_Other);
    }

    void CVoxelSpace::insert(const pair &_pair)
    {
        Math::Vec3i position = chunkpos(_pair.first);
        auto it = m_Chunks.find(position);

        // Creates a new chunk, if neccessary
        if(it == m_Chunks.end())
            it = m_Chunks.insert({position, CChunk(m_ChunkSize)}).first;

        it->second.insert(_pair, CBBox(position, m_ChunkSize));
        m_VoxelsCount++;
    }

    CVoxelSpace::iterator CVoxelSpace::erase(const iterator &_it)
    {
        Math::Vec3i position = chunkpos(_it->first);
        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return end();

        auto res = it->second.erase(_it, CBBox(position, m_ChunkSize));
        m_VoxelsCount--;

        // Searches for the next voxel inside of any chunk.
        while (!res.second)
        {
            it++;
            if(it == m_Chunks.end())
                return end();

            res = it->second.next(it->second.inner_bbox(it->first).Beg, CBBox(it->first, m_ChunkSize));
        }
        
        if(res.second)
            return CVoxelSpaceIterator(this, it->second.inner_bbox(it->first), res);

        return end();
    }

    CVoxelSpace::iterator CVoxelSpace::find(const Math::Vec3i &_v) const
    {
        Math::Vec3i position = chunkpos(_v);
        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return end();

        CVoxel *vox = it->second.find(_v, CBBox(position, m_ChunkSize));
        if(!vox)
            return end();

        return CVoxelSpaceIterator(this, it->second.inner_bbox(it->first), {_v, vox});
    }

    CVoxelSpace::iterator CVoxelSpace::find(const Math::Vec3i &_v, bool _Opaque) const
    {
        Math::Vec3i position = chunkpos(_v);
        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return end();

        CVoxel *vox = it->second.find(_v, CBBox(position, m_ChunkSize), _Opaque);
        if(!vox)
            return end();

        return CVoxelSpaceIterator(this, it->second.inner_bbox(it->first), {_v, vox});
    }

    CVoxelSpace::iterator CVoxelSpace::findVisible(const Math::Vec3i &_v) const
    {
        Math::Vec3i position = chunkpos(_v);
        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return end();

        CVoxel *vox = it->second.findVisible(_v, CBBox(position, m_ChunkSize));
        if(!vox)
            return end();

        return CVoxelSpaceIterator(this, it->second.inner_bbox(it->first), {_v, vox});
    }

    CVoxelSpace::iterator CVoxelSpace::findVisible(const Math::Vec3i &_v, bool _Opaque) const
    {
        static std::pair<Math::Vec3i, const CChunk *> last(Math::Vec3i(), nullptr);

        Math::Vec3i position = chunkpos(_v);

        if(!(last.first == position && last.second))
        {
            auto it = m_Chunks.find(position);
            if(it == m_Chunks.end())
                return end();

            last.first = position;
            last.second = &it->second;
        }

        CVoxel *vox = last.second->findVisible(_v, CBBox(position, m_ChunkSize), _Opaque);
        if(!vox)
            return end();

        return CVoxelSpaceIterator(this, last.second->inner_bbox(last.first), {_v, vox});
    }


    VectoriMap<Voxel> CVoxelSpace::queryVisible(bool opaque) const
    {
        VectoriMap<Voxel> ret;
        for (auto &&c : m_Chunks)
        {
            auto bbox = CBBox(c.first, m_ChunkSize);
            auto inner = c.second.inner_bbox(bbox.Beg);
            for (int z = inner.Beg.z; z < inner.End.z; z++)
            {
                for (int y = inner.Beg.y; y < inner.End.y; y++)
                {
                    for (int x = inner.Beg.x; x < inner.End.x; x++)
                    {
                        auto res = c.second.find(Math::Vec3i(x, y, z), bbox);
                        if(res && res->IsVisible() && res->Transparent == !opaque)
                            ret.insert({Math::Vec3i(x, y, z), res});
                    }
                }
            }     
        }
        return ret;
    }

    std::list<SChunkMeta> CVoxelSpace::queryDirtyChunks() const
    {
        std::list<SChunkMeta> ret;
        int counter = 0;
        for (auto &&c : m_Chunks)
        {
            if(c.second.IsDirty)
                ret.push_back({(size_t)&c.second, &c.second, CBBox(c.first, c.first + m_ChunkSize), c.second.inner_bbox(c.first)});
        }
        return ret;
    }

    void CVoxelSpace::markAsProcessed(const SChunkMeta &_Chunk)
    {
        auto it = m_Chunks.find(_Chunk.TotalBBox.Beg);
        if(it != m_Chunks.end())
            it->second.IsDirty = false;
    }

    std::list<SChunkMeta> CVoxelSpace::queryChunks() const
    {
        std::list<SChunkMeta> ret;
        for (auto &&c : m_Chunks)
            ret.push_back({(size_t)&c.second, &c.second, CBBox(c.first, c.first + m_ChunkSize), c.second.inner_bbox(c.first)});
        return ret;
    }

    void CVoxelSpace::generateVisibilityMask()
    {
        const static std::vector<std::pair<Math::Vec3f, int>> AXIS_DIRECTIONS = {
            {Math::Vec3f(1, 0, 0), 0},
            {Math::Vec3f(0, 1, 0), 1},
            {Math::Vec3f(0, 0, 1), 2}
        };

        for (auto &&c : m_Chunks)
        {
            if(!c.second.IsDirty)
                continue;

            auto bbox = CBBox(c.first, m_ChunkSize);
            auto inner = c.second.inner_bbox(bbox.Beg);
            
            Math::Vec3i beg = inner.Beg;
            Math::Vec3i end = inner.End;            

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
                        Math::Vec3i pos;
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

    CVoxelSpace::iterator CVoxelSpace::next(const Math::Vec3i &_FromPosition) const
    {
        Math::Vec3i position = chunkpos(_FromPosition);
        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return end();

        auto res = it->second.next(_FromPosition, CBBox(position, m_ChunkSize));
        
        // Searches for the next voxel inside of any chunk.
        while (!res.second)
        {
            it++;
            if(it == m_Chunks.end())
                return end();

            res = it->second.next(it->second.inner_bbox(it->first).Beg, CBBox(it->first, m_ChunkSize));
        }

        if(res.second)
            return CVoxelSpaceIterator(this, it->second.inner_bbox(it->first), res);

        return end();
    }

    void CVoxelSpace::updateVisibility(const Math::Vec3i &_Position)
    {
        auto it = this->find(_Position);
        if(it == this->end())
        {
            const static std::vector<std::pair<Math::Vec3f, CVoxel::Visibility>> DIRECTIONS = {
                {Math::Vec3f(1, 0, 0), CVoxel::Visibility::LEFT},
                {Math::Vec3f(-1, 0, 0), CVoxel::Visibility::RIGHT},
                {Math::Vec3f(0, 1, 0), CVoxel::Visibility::BACKWARD},
                {Math::Vec3f(0, -1, 0), CVoxel::Visibility::FORWARD},
                {Math::Vec3f(0, 0, -1), CVoxel::Visibility::UP},
                {Math::Vec3f(0, 0, 1), CVoxel::Visibility::DOWN}
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
            const static std::vector<std::pair<Math::Vec3f, int>> AXIS_DIRECTIONS = {
                {Math::Vec3f(1, 0, 0), 0},
                {Math::Vec3f(0, 1, 0), 1},
                {Math::Vec3f(0, 0, 1), 2}
            };

            for (auto &&axis : AXIS_DIRECTIONS)
            {
                Math::Vec3f start = _Position - axis.first;
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
        return CVoxelSpaceIterator(this, bbox, it->second.next(bbox.Beg, CBBox(it->first, m_ChunkSize)));
    }

    CVoxelSpace::iterator CVoxelSpace::end() const
    {
        return CVoxelSpaceIterator(this, CBBox(), {Math::Vec3i(), nullptr});
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

    Math::Vec3i CVoxelSpace::chunkpos(const Math::Vec3i &_Position) const
    {
        // const static uint32_t mask = 0xFFFFFFF0;
        // Math::Vec3i(_Position.x & mask, _Position.y & mask, _Position.z & mask);

        return Math::floor(Math::Vec3f(_Position) / m_ChunkSize) * m_ChunkSize;
    }

    //////////////////////////////////////////////////
    // CVoxelSpace::CChunk functions
    //////////////////////////////////////////////////

    CChunk::CChunk(const Math::Vec3i &_ChunkSize) : m_InnerBBox(Math::Vec3i(INT32_MAX, INT32_MAX, INT32_MAX), Math::Vec3i()), IsDirty(false)
    {
        m_Data = new CVoxel[_ChunkSize.x * _ChunkSize.y * _ChunkSize.z];   
    }

    CChunk::CChunk(CChunk &&_Other) : m_Data(nullptr)
    {
        *this = std::move(_Other);
    }

    void CChunk::insert(const pair &_pair, const CBBox &_ChunkDim)
    {
        Math::Vec3i relPos = (_pair.first - _ChunkDim.Beg).abs();
        m_Data[relPos.x + _ChunkDim.End.x * relPos.y + _ChunkDim.End.x * _ChunkDim.End.y * relPos.z] = _pair.second;
    
        m_InnerBBox.Beg = m_InnerBBox.Beg.min(relPos);
        m_InnerBBox.End = m_InnerBBox.End.max(relPos);
        IsDirty = true;
    }

    CVoxelSpace::ppair CChunk::erase(const iterator &_it, const CBBox &_ChunkDim)
    {
        Math::Vec3i relPos = (_it->first - _ChunkDim.Beg).abs();
        m_Data[relPos.x + _ChunkDim.End.x * relPos.y + _ChunkDim.End.x * _ChunkDim.End.y * relPos.z] = CVoxel();
        
        return next(_it->first, _ChunkDim);
    }

    CVoxelSpace::ppair CChunk::next(const Math::Vec3i &_Position, const CBBox &_ChunkDim) const
    {
        Math::Vec3i relPos = (_Position - _ChunkDim.Beg).abs();
        for (int z = relPos.z; z < m_InnerBBox.End.z; z++)
        {
            for (int y = relPos.y; y < m_InnerBBox.End.y; y++)
            {
                for (int x = relPos.x; x < m_InnerBBox.End.x; x++)
                {
                    CVoxel &vox = m_Data[x + _ChunkDim.End.x * y + _ChunkDim.End.x * _ChunkDim.End.y * z];
                    if(vox.IsVisible())
                        return {_ChunkDim.Beg + Math::Vec3i(x, y, z), &vox};
                }
            }
        }
        
        return {Math::Vec3i(), nullptr};
    }

    Voxel CChunk::find(const Math::Vec3i &_v, const CBBox &_ChunkDim) const
    {
        Math::Vec3i relPos = (_v - _ChunkDim.Beg).abs();
        CVoxel &vox = m_Data[relPos.x + _ChunkDim.End.x * relPos.y + _ChunkDim.End.x * _ChunkDim.End.y * relPos.z];
        if(vox.IsInstantiated())
            return &vox;

        return nullptr;
    }

    Voxel CChunk::find(const Math::Vec3i &_v, const CBBox &_ChunkDim, bool _Opaque) const
    {
        Math::Vec3i relPos = (_v - _ChunkDim.Beg).abs();
        CVoxel &vox = m_Data[relPos.x + _ChunkDim.End.x * relPos.y + _ChunkDim.End.x * _ChunkDim.End.y * relPos.z];
        if(vox.IsInstantiated() && (vox.Transparent != _Opaque))
            return &vox;

        return nullptr;
    }

    Voxel CChunk::findVisible(const Math::Vec3i &_v, const CBBox &_ChunkDim) const
    {
        Math::Vec3i relPos = (_v - _ChunkDim.Beg).abs();
        CVoxel &vox = m_Data[relPos.x + _ChunkDim.End.x * relPos.y + _ChunkDim.End.x * _ChunkDim.End.y * relPos.z];
        if(vox.IsInstantiated() && vox.IsVisible())
            return &vox;

        return nullptr;
    }

    Voxel CChunk::findVisible(const Math::Vec3i &_v, const CBBox &_ChunkDim, bool _Opaque) const
    {
        Math::Vec3i relPos = (_v - _ChunkDim.Beg).abs();
        CVoxel &vox = m_Data[relPos.x + _ChunkDim.End.x * relPos.y + _ChunkDim.End.x * _ChunkDim.End.y * relPos.z];
        if(vox.IsInstantiated() && vox.IsVisible() && (vox.Transparent != _Opaque))
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

        _Other.m_Data = nullptr;
        _Other.m_InnerBBox = CBBox();
        _Other.IsDirty = false;

        return *this;
    }

    //////////////////////////////////////////////////
    // CVoxelSpaceIterator functions
    //////////////////////////////////////////////////

    CVoxelSpaceIterator::CVoxelSpaceIterator() : m_Space(nullptr), m_Pair(Math::Vec3i(), nullptr) { }
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
        _Other.m_Pair = {Math::Vec3i(), nullptr};
        _Other.m_InnerBox = CBBox();

        return *this;
    }
}