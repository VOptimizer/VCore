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

#include <VCore/Voxel/VoxelSpace.hpp>
#include <VCore/Voxel/VoxelModel.hpp>

namespace VCore
{
    //////////////////////////////////////////////////
    // CChunkQueryList functions
    //////////////////////////////////////////////////

    CChunkQueryList::iterator CChunkQueryList::begin()
    {
        auto it = CChunkQueryIterator(this, m_Chunks->begin());

        // Filters until the first none filtered element is reached
        if(m_FilterFunction)
            it.InitFilter();
        else if(m_Chunks->begin() != m_Chunks->end())
        {
            Math::Vec3iHasher hasher;

            CBBox bbox(it.m_Iterator->first, it.m_Iterator->first + m_ChunkSize);
            it.m_ChunkMeta = {hasher(it.m_Iterator->first), &it.m_Iterator->second, bbox, it.m_Iterator->second.inner_bbox(it.m_Iterator->first)};
        }

        return it;
    }

    CChunkQueryList::iterator CChunkQueryList::end()
    {
        return CChunkQueryIterator(this, m_Chunks->end());
    }

    CChunkQueryList::iterator CChunkQueryList::begin() const
    {
        auto it = CChunkQueryIterator(this, m_Chunks->begin());

        // Filters until the first none filtered element is reached
        if(m_FilterFunction)
            it.InitFilter();
        else
        {
            Math::Vec3iHasher hasher;
            CBBox bbox(it.m_Iterator->first, it.m_Iterator->first + m_ChunkSize);
            it.m_ChunkMeta = {hasher(it.m_Iterator->first), &it.m_Iterator->second, bbox, it.m_Iterator->second.inner_bbox(it.m_Iterator->first)};
        }
        return it;
    }

    CChunkQueryList::iterator CChunkQueryList::end() const
    {
        return CChunkQueryIterator(this, m_Chunks->end());
    }
    
    CChunkQueryList::operator std::vector<SChunkMeta>() const
    {
        std::vector<SChunkMeta> ret;

        auto begIT = begin();
        auto endIT = end();

        while(begIT != endIT)
        {
            ret.push_back(*begIT);
            begIT++;
        }

        return ret;
    }

    CChunkQueryList &CChunkQueryList::operator=(const CChunkQueryList &_Other)
    {
        m_Chunks = _Other.m_Chunks;
        m_FilterFunction = _Other.m_FilterFunction;
        m_Userdata = _Other.m_Userdata;
        m_ChunkSize = _Other.m_ChunkSize;
        return *this;
    }

    CChunkQueryList &CChunkQueryList::operator=(CChunkQueryList &&_Other)
    {
        m_Chunks = _Other.m_Chunks;
        m_FilterFunction = _Other.m_FilterFunction;
        m_Userdata = _Other.m_Userdata;
        m_ChunkSize = _Other.m_ChunkSize;

        _Other.m_Chunks = nullptr;
        _Other.m_FilterFunction = nullptr;
        _Other.m_Userdata = nullptr;
        _Other.m_ChunkSize = Math::Vec3i();
        return *this;
    }

    bool CChunkQueryList::ApplyFilter(ankerl::unordered_dense::map<Math::Vec3i, CChunk, Math::Vec3iHasher>::const_iterator &_Iterator, SChunkMeta &_ChunkMeta) const
    {
        CBBox bbox(_Iterator->first, _Iterator->first + m_ChunkSize);
        bool filtered = !m_FilterFunction;
        if(m_FilterFunction)
            filtered = m_FilterFunction(bbox, _Iterator->second, m_Userdata);

        if(filtered)
        {
            Math::Vec3iHasher hasher;
            _ChunkMeta = {hasher(_Iterator->first), &_Iterator->second, bbox, _Iterator->second.inner_bbox(_Iterator->first)};
        }

        return filtered;
    }

    SChunkMeta CChunkQueryList::FilterNext(ankerl::unordered_dense::map<Math::Vec3i, CChunk, Math::Vec3iHasher>::const_iterator &_Iterator) const
    {
        while (true)
        {
            _Iterator++;
            if(_Iterator == m_Chunks->end())
                return SChunkMeta();

            SChunkMeta result;
            if(ApplyFilter(_Iterator, result))
                return result;
        }
    }

    //////////////////////////////////////////////////
    // CChunkQueryList::CChunkQueryIterator functions
    //////////////////////////////////////////////////

    CChunkQueryList::CChunkQueryIterator::reference CChunkQueryList::CChunkQueryIterator::operator*() const
    {
        return m_ChunkMeta;
    }

    CChunkQueryList::CChunkQueryIterator::pointer CChunkQueryList::CChunkQueryIterator::operator->() const
    {
        return &m_ChunkMeta;
    }

    void CChunkQueryList::CChunkQueryIterator::InitFilter()
    {
        if(!m_Parent->ApplyFilter(m_Iterator, m_ChunkMeta))
            m_ChunkMeta = m_Parent->FilterNext(m_Iterator);
    }

    CChunkQueryList::CChunkQueryIterator& CChunkQueryList::CChunkQueryIterator::operator++()
    {
        m_ChunkMeta = m_Parent->FilterNext(m_Iterator);
        return *this;
    }

    CChunkQueryList::CChunkQueryIterator& CChunkQueryList::CChunkQueryIterator::operator++(int)
    {
        m_ChunkMeta = m_Parent->FilterNext(m_Iterator);
        return *this;
    }

    bool CChunkQueryList::CChunkQueryIterator::operator!=(const CChunkQueryIterator &_Rhs)
    {
        return m_Iterator != _Rhs.m_Iterator;
    }

    bool CChunkQueryList::CChunkQueryIterator::operator==(const CChunkQueryIterator &_Rhs)
    {
        return m_Iterator == _Rhs.m_Iterator;
    }

    CChunkQueryList::CChunkQueryIterator& CChunkQueryList::CChunkQueryIterator::operator=(const CChunkQueryIterator &_Other)
    {
        m_ChunkMeta = _Other.m_ChunkMeta;
        m_Parent = _Other.m_Parent;
        m_Iterator = _Other.m_Iterator;
        return *this;
    }

    CChunkQueryList::CChunkQueryIterator& CChunkQueryList::CChunkQueryIterator::operator=(CChunkQueryIterator &&_Other)
    {
        m_ChunkMeta = _Other.m_ChunkMeta;
        m_Parent = _Other.m_Parent;
        _Other.m_Parent = nullptr;
        m_Iterator = std::move(_Other.m_Iterator);
        return *this;
    }

    //////////////////////////////////////////////////
    // CVoxelSpace functions
    //////////////////////////////////////////////////

    CVoxelSpace::CVoxelSpace() : m_ChunkSize(16, 16, 16), m_VoxelsCount(0) {}
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

        it->second.insert(this, _pair, CBBox(position, m_ChunkSize));
        m_VoxelsCount++;
    }

    CVoxelSpace::iterator CVoxelSpace::erase(const iterator &_it)
    {
        Math::Vec3i position = chunkpos(_it->first);
        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return end();

        auto res = it->second.erase(this, _it, CBBox(position, m_ChunkSize));
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

    CVoxelSpace::querylist CVoxelSpace::queryDirtyChunks() const
    {
        return CChunkQueryList(m_Chunks, m_ChunkSize, [](const CBBox &_BBox, const CChunk &_Chunk, void *_Userdata)
        {
            (void)_BBox;
            (void)_Userdata;
            return _Chunk.IsDirty;
        });
    }

    void CVoxelSpace::markAsProcessed(const SChunkMeta &_Chunk)
    {
        auto it = m_Chunks.find(_Chunk.TotalBBox.Beg);
        if(it != m_Chunks.end())
            it->second.IsDirty = false;
    }

    CVoxelSpace::querylist CVoxelSpace::queryChunks() const
    {
        return CChunkQueryList(m_Chunks, m_ChunkSize);
    }

    CVoxelSpace::querylist CVoxelSpace::queryChunks(const CFrustum *_Frustum) const
    {
        return CChunkQueryList(m_Chunks, m_ChunkSize, [](const CBBox &_BBox, const CChunk &_Chunk, void *_Userdata)
        {
            CFrustum *frustum = (CFrustum*)_Userdata;
            return frustum->IsOnFrustum(_Chunk.inner_bbox(_BBox.Beg));
        }, const_cast<CFrustum*>(_Frustum));
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
        m_ChunkSize = _Other.m_ChunkSize;
        m_VoxelsCount = _Other.m_VoxelsCount;
        m_Chunks = std::move(_Other.m_Chunks);

        return *this;
    }

    CChunk *CVoxelSpace::GetChunk(const Math::Vec3i &_Position)
    {
        auto position = chunkpos(_Position);
        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return nullptr;

        return &it->second;
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

    CChunk::CChunk(const Math::Vec3i &_ChunkSize) : IsDirty(false), m_InnerBBox(Math::Vec3i(INT32_MAX, INT32_MAX, INT32_MAX), Math::Vec3i())
    {
        m_Data = new CVoxel[_ChunkSize.x * _ChunkSize.y * _ChunkSize.z];   
    }

    CChunk::CChunk(CChunk &&_Other) : m_Data(nullptr)
    {
        *this = std::move(_Other);
    }

    void CChunk::CheckAndUpdateVisibility(CVoxelSpace *_Space, const CBBox &_ChunkDim, Voxel _ThisVoxel, const Math::Vec3i &_Pos, CVoxel::Visibility _This, CVoxel::Visibility _Other)
    {
        // 1. Both opaque touching faces invisible
        // 2. One transparent touching faces visible
        // 3. Both transparent different transparency touching faces invisible

        Voxel other = GetBlock(_Space, _ChunkDim, _Pos);
        if(other && other->IsInstantiated())
        {
            if(!_ThisVoxel->IsInstantiated())
            {
                other->VisibilityMask |= ~_Other;
                _ThisVoxel->VisibilityMask &= _This;
                return;
            }

            // 1. + 3.
            if(other->Transparent == _ThisVoxel->Transparent)
            {
                other->VisibilityMask &= _Other;
                _ThisVoxel->VisibilityMask &= _This;
            }
            else if(!other->Transparent && _ThisVoxel->Transparent)    // 2.
            {
                other->VisibilityMask |= ~_Other;
                _ThisVoxel->VisibilityMask &= _This;
            }
            else if(other->Transparent && !_ThisVoxel->Transparent)    // 2.
            {
                other->VisibilityMask &= _Other;
                _ThisVoxel->VisibilityMask |= ~_This;
            }
        }
        else if(!_ThisVoxel->IsInstantiated())
            _ThisVoxel->VisibilityMask &= _This;
        else
            _ThisVoxel->VisibilityMask |= ~_This;
    }

    CVoxel *CChunk::GetBlock(CVoxelSpace *_Space, const CBBox &_ChunkDim, const Math::Vec3i &_v)
    {
        if((_v.x >= 0 && _v.y >= 0 && _v.z >= 0) && (_v.x < _ChunkDim.End.x && _v.y < _ChunkDim.End.y && _v.z < _ChunkDim.End.z))
            return &m_Data[_v.x + _ChunkDim.End.x * _v.y + _ChunkDim.End.x * _ChunkDim.End.y * _v.z];
        else
        {
            auto globalPos =  _ChunkDim.Beg + _v;
            auto chunk = _Space->GetChunk(globalPos);
            if(chunk)
            {
                chunk->IsDirty = true;
                auto chunkpos = _Space->chunkpos(globalPos);
                return chunk->GetBlock(_Space, CBBox(chunkpos, _ChunkDim.End), globalPos - chunkpos);
            }
        }

        return nullptr;
    }

    void CChunk::insert(CVoxelSpace *_Space, const pair &_pair, const CBBox &_ChunkDim)
    {
        Math::Vec3i relPos = (_pair.first - _ChunkDim.Beg).abs();
        CVoxel &voxel = m_Data[relPos.x + _ChunkDim.End.x * relPos.y + _ChunkDim.End.x * _ChunkDim.End.y * relPos.z]; //= _pair.second;
        voxel.Color = _pair.second.Color;
        voxel.Material = _pair.second.Material;
        voxel.Transparent = _pair.second.Transparent;

        if(!voxel.IsVisible())
            voxel.VisibilityMask = CVoxel::Visibility::VISIBLE;

        CheckAndUpdateVisibility(_Space, _ChunkDim, &voxel, relPos + Math::Vec3i::UP, ~CVoxel::Visibility::UP, ~CVoxel::Visibility::DOWN);
        CheckAndUpdateVisibility(_Space, _ChunkDim, &voxel, relPos + Math::Vec3i::DOWN, ~CVoxel::Visibility::DOWN, ~CVoxel::Visibility::UP);

        CheckAndUpdateVisibility(_Space, _ChunkDim, &voxel, relPos + Math::Vec3i::LEFT, ~CVoxel::Visibility::LEFT, ~CVoxel::Visibility::RIGHT);
        CheckAndUpdateVisibility(_Space, _ChunkDim, &voxel, relPos + Math::Vec3i::RIGHT, ~CVoxel::Visibility::RIGHT, ~CVoxel::Visibility::LEFT);

        CheckAndUpdateVisibility(_Space, _ChunkDim, &voxel, relPos + Math::Vec3i::FRONT, ~CVoxel::Visibility::FORWARD, ~CVoxel::Visibility::BACKWARD);
        CheckAndUpdateVisibility(_Space, _ChunkDim, &voxel, relPos + Math::Vec3i::BACK, ~CVoxel::Visibility::BACKWARD, ~CVoxel::Visibility::FORWARD);

        m_InnerBBox.Beg = m_InnerBBox.Beg.min(relPos);
        m_InnerBBox.End = m_InnerBBox.End.max(relPos);
        IsDirty = true;
    }

    CVoxelSpace::ppair CChunk::erase(CVoxelSpace *_Space, const iterator &_it, const CBBox &_ChunkDim)
    {
        Math::Vec3i relPos = (_it->first - _ChunkDim.Beg).abs();
        // m_Data[relPos.x + _ChunkDim.End.x * relPos.y + _ChunkDim.End.x * _ChunkDim.End.y * relPos.z] = CVoxel();
        CVoxel &voxel = m_Data[relPos.x + _ChunkDim.End.x * relPos.y + _ChunkDim.End.x * _ChunkDim.End.y * relPos.z];
        voxel = CVoxel();
        IsDirty = true;

        CheckAndUpdateVisibility(_Space, _ChunkDim, &voxel, relPos + Math::Vec3i::UP, ~CVoxel::Visibility::UP, ~CVoxel::Visibility::DOWN);
        CheckAndUpdateVisibility(_Space, _ChunkDim, &voxel, relPos + Math::Vec3i::DOWN, ~CVoxel::Visibility::DOWN, ~CVoxel::Visibility::UP);

        CheckAndUpdateVisibility(_Space, _ChunkDim, &voxel, relPos + Math::Vec3i::LEFT, ~CVoxel::Visibility::LEFT, ~CVoxel::Visibility::RIGHT);
        CheckAndUpdateVisibility(_Space, _ChunkDim, &voxel, relPos + Math::Vec3i::RIGHT, ~CVoxel::Visibility::RIGHT, ~CVoxel::Visibility::LEFT);

        CheckAndUpdateVisibility(_Space, _ChunkDim, &voxel, relPos + Math::Vec3i::FRONT, ~CVoxel::Visibility::FORWARD, ~CVoxel::Visibility::BACKWARD);
        CheckAndUpdateVisibility(_Space, _ChunkDim, &voxel, relPos + Math::Vec3i::BACK, ~CVoxel::Visibility::BACKWARD, ~CVoxel::Visibility::FORWARD);
        
        // Checks if the bbox must be resized
        for (size_t i = 0; i < 3; i++)
        {
            if(relPos.v[i] == (_ChunkDim.End.v[i] - 1))
                m_InnerBBox.End.v[i] -= 1;
            else if(relPos.v[i] == 0)
                m_InnerBBox.Beg.v[i] += 1;
        }
        
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

    typename CVoxelSpaceIterator::reference CVoxelSpaceIterator::operator*() const
    {
        return m_Pair;
    }

    typename CVoxelSpaceIterator::pointer CVoxelSpaceIterator::operator->() const
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