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

#include <VCore/Formats/Streamable.hpp>
#include <VCore/Voxel/Storage/VoxelSpace.hpp>
#include <VCore/Voxel/VoxelModel.hpp>
#include <VCore/VConfig.hpp>

namespace VCore
{
    static const Math::Vec3i CHUNK_SIZE(Config::ChunkSize, Config::ChunkSize, Config::ChunkSize);

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

            CBBox bbox(it.m_Iterator->first, it.m_Iterator->first + CHUNK_SIZE);
            it.m_ChunkMeta = {hasher(it.m_Iterator->first), it.m_Iterator->second, bbox, it.m_Iterator->second->inner_bbox(it.m_Iterator->first)};
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
            CBBox bbox(it.m_Iterator->first, it.m_Iterator->first + CHUNK_SIZE);
            it.m_ChunkMeta = {hasher(it.m_Iterator->first), it.m_Iterator->second, bbox, it.m_Iterator->second->inner_bbox(it.m_Iterator->first)};
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
        return *this;
    }

    CChunkQueryList &CChunkQueryList::operator=(CChunkQueryList &&_Other)
    {
        m_Chunks = _Other.m_Chunks;
        m_FilterFunction = _Other.m_FilterFunction;
        m_Userdata = _Other.m_Userdata;

        _Other.m_Chunks = nullptr;
        _Other.m_FilterFunction = nullptr;
        _Other.m_Userdata = nullptr;
        return *this;
    }

    bool CChunkQueryList::ApplyFilter(ankerl::unordered_dense::map<Math::Vec3i, CChunk*, Math::Vec3iHasher>::const_iterator &_Iterator, SChunkMeta &_ChunkMeta) const
    {
        CBBox bbox(_Iterator->first, _Iterator->first + CHUNK_SIZE);
        bool filtered = !m_FilterFunction;
        if(m_FilterFunction)
            filtered = m_FilterFunction(bbox, _Iterator->second, m_Userdata);

        if(filtered)
        {
            Math::Vec3iHasher hasher;
            _ChunkMeta = {hasher(_Iterator->first), _Iterator->second, bbox, _Iterator->second->inner_bbox(_Iterator->first)};
        }

        return filtered;
    }

    SChunkMeta CChunkQueryList::FilterNext(ankerl::unordered_dense::map<Math::Vec3i, CChunk*, Math::Vec3iHasher>::const_iterator &_Iterator) const
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

    CVoxelSpace::CVoxelSpace(IStreamable *_Stream) : m_VoxelsCount(0), m_Stream(_Stream), m_ModelLoaded(false) {}
    CVoxelSpace::CVoxelSpace(CVoxelSpace &&_Other) { *this = std::move(_Other); }

    CVoxelSpace::~CVoxelSpace() 
    { 
        clear(); 
        if(m_Stream) 
            delete m_Stream;
    }

    void CVoxelSpace::insert(const pair &_pair)
    {
        CheckLoadModel();

        Math::Vec3i position = GetChunkpos(_pair.first);
        auto it = m_Chunks.find(position);

        // Creates a new chunk, if neccessary
        if(it == m_Chunks.end())
            it = m_Chunks.insert({position, new CChunk(this)}).first;

        // Time to upgrade
        if(!it->second->insert(_pair))
        {
            it->second->Upgrade();
            it->second->insert(_pair);
        }

        m_VoxelsCount++;
    }

    CVoxelSpace::iterator CVoxelSpace::erase(const iterator &_it)
    {
        CheckLoadModel();

        Math::Vec3i position = GetChunkpos(_it->first);
        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return end();

        auto res = it->second->erase(_it);
        m_VoxelsCount--;

        // Removes the empty chunk.
        if(it->second->inner_bbox(position).GetSize() == Math::Vec3i::ZERO)
        {
            delete it->second;
            it = m_Chunks.erase(it);
        }

        if(it != m_Chunks.end())
        {
            // Searches for the next voxel inside of any chunk.
            while (!res.second.IsInstantiated())
            {
                it++;
                if(it == m_Chunks.end())
                    return end();

                res = it->second->next(it->second->inner_bbox(it->first).Beg);
            }
            
            if(res.second.IsInstantiated())
                return CVoxelSpaceIterator(this, it->second->inner_bbox(it->first), res);
        }

        return end();
    }

    CVoxelSpace::iterator CVoxelSpace::find(const Math::Vec3i &_v) const
    {
        const_cast<CVoxelSpace*>(this)->CheckLoadModel();
        Math::Vec3i position = GetChunkpos(_v);
        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return end();

        CVoxel vox = it->second->find(_v);
        if(!vox.IsInstantiated())
            return end();

        return CVoxelSpaceIterator(this, it->second->inner_bbox(it->first), {_v, vox});
    }

    CVoxelSpace::querylist CVoxelSpace::queryDirtyChunks() const
    {
        const_cast<CVoxelSpace*>(this)->CheckLoadModel();
        return CChunkQueryList(m_Chunks, [](const CBBox &_BBox, const CChunk *_Chunk, void *_Userdata)
        {
            (void)_BBox;
            (void)_Userdata;
            return _Chunk->IsDirty;
        });
    }

    void CVoxelSpace::markAsProcessed(const SChunkMeta &_Chunk)
    {
        auto it = m_Chunks.find(_Chunk.TotalBBox.Beg);
        if(it != m_Chunks.end())
            it->second->IsDirty = false;
    }

    CVoxelSpace::querylist CVoxelSpace::queryChunks() const
    {
        const_cast<CVoxelSpace*>(this)->CheckLoadModel();
        return CChunkQueryList(m_Chunks);
    }

    CVoxelSpace::querylist CVoxelSpace::queryChunks(const CFrustum *_Frustum) const
    {
        const_cast<CVoxelSpace*>(this)->CheckLoadModel();
        return CChunkQueryList(m_Chunks, [](const CBBox &_BBox, const CChunk *_Chunk, void *_Userdata)
        {
            CFrustum *frustum = (CFrustum*)_Userdata;
            return frustum->IsOnFrustum(_Chunk->inner_bbox(_BBox.Beg));
        }, const_cast<CFrustum*>(_Frustum));
    }

    CVoxelSpace::iterator CVoxelSpace::next(const Math::Vec3i &_FromPosition) const
    {
        Math::Vec3i position = GetChunkpos(_FromPosition);
        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return end();

        auto res = it->second->next(_FromPosition);
        
        // Searches for the next voxel inside of any chunk.
        while (!res.second.IsInstantiated())
        {
            it++;
            if(it == m_Chunks.end())
                return end();

            res = it->second->next(it->second->inner_bbox(it->first).Beg);
        }

        if(res.second.IsInstantiated())
            return CVoxelSpaceIterator(this, it->second->inner_bbox(it->first), res);

        return end();
    }

    void CVoxelSpace::CheckLoadModel()
    {
        if(m_Stream && !m_ModelLoaded && !m_Stream->SupportsChunkOffloading())
        {
            m_ModelLoaded = true;
            m_Stream->ReadVoxelSpace(*this);
        }
    }

    CVoxelSpace::iterator CVoxelSpace::begin()
    {
        const_cast<CVoxelSpace*>(this)->CheckLoadModel();
        if(m_Chunks.empty())
            return end();

        auto it = m_Chunks.begin();
        auto bbox = it->second->inner_bbox(it->first);
        return CVoxelSpaceIterator(this, bbox, it->second->next(bbox.Beg));
    }

    CVoxelSpace::iterator CVoxelSpace::end() const
    {
        return CVoxelSpaceIterator(this, CBBox(), {Math::Vec3i(), CVoxel()});
    }

    CBBox CVoxelSpace::calculateBBox() const
    {
        const_cast<CVoxelSpace*>(this)->CheckLoadModel();

        CBBox bbox(Math::Vec3i(INT32_MAX, INT32_MAX, INT32_MAX), Math::Vec3i());
        for (auto &&c : m_Chunks)
        {
            auto innerBBox = c.second->inner_bbox(c.first);
            bbox.Beg = innerBBox.Beg.min(bbox.Beg);
            bbox.End = innerBBox.End.max(bbox.End);
        }

        return bbox;
    }

    CChunk* CVoxelSpace::createOrGetChunk(const Math::Vec3i &_Position)
    {
        CheckLoadModel();

        Math::Vec3i position = GetChunkpos(_Position);
        auto it = m_Chunks.find(position);

        // Creates a new chunk, if neccessary
        if(it == m_Chunks.end())
            it = m_Chunks.insert({position, new CChunk(this)}).first;

        return it->second;
    }

    void CVoxelSpace::clear()
    {
        for (auto &&chunk : m_Chunks)
            delete chunk.second;
        
        m_Chunks.clear();
        m_ModelLoaded = false;
    }

    CVoxelSpace &CVoxelSpace::operator=(CVoxelSpace &&_Other)
    {
        m_VoxelsCount = _Other.m_VoxelsCount;
        m_Chunks = std::move(_Other.m_Chunks);
        m_ModelLoaded = std::move(_Other.m_ModelLoaded);

        return *this;
    }

    void CVoxelSpace::SetStream(IStreamable *_Strm)
    {
        if(m_Stream)
            delete m_Stream;

        m_Stream = _Strm;
    }

    CChunk *CVoxelSpace::GetChunk(const Math::Vec3i &_Position) const
    {
        auto position = GetChunkpos(_Position);
        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return nullptr;

        return it->second;
    }
}