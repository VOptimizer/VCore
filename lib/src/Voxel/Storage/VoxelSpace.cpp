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

#include "VCore/Math/Vector.hpp"
#include "VCore/Voxel/Storage/Chunk.hpp"
#include <VCore/Math/Mat4x4.hpp>
#include <VCore/Voxel/Frustum.hpp>
#include <VCore/Formats/Streamable.hpp>
#include <VCore/Voxel/Storage/VoxelSpace.hpp>
#include <VCore/Voxel/VoxelModel.hpp>
#include <VCore/VConfig.hpp>
#include <utility>

namespace VCore
{
    // struct ChunkCache
    // {
    //     std::pair<Math::Vec3i, CChunk*> Chunks[9]{};
    // };

    // thread_local ChunkCache g_Cache;

    struct FrustumQuery
    {
        const CFrustum *Frustum;
        Math::Mat4x4 ModelMatrix;
        IStreamable *Source;
    };

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

    CChunkQueryList &CChunkQueryList::operator=(const CChunkQueryList &p_Other)
    {
        m_Chunks = p_Other.m_Chunks;
        m_FilterFunction = p_Other.m_FilterFunction;
        m_Userdata = p_Other.m_Userdata;
        return *this;
    }

    CChunkQueryList &CChunkQueryList::operator=(CChunkQueryList &&p_Other)
    {
        m_Chunks = p_Other.m_Chunks;
        m_FilterFunction = p_Other.m_FilterFunction;
        m_Userdata = p_Other.m_Userdata;

        p_Other.m_Chunks = nullptr;
        p_Other.m_FilterFunction = nullptr;
        p_Other.m_Userdata = nullptr;
        return *this;
    }

    bool CChunkQueryList::ApplyFilter(ankerl::unordered_dense::map<Math::Vec3i, CChunk*, Math::Vec3iHasher>::const_iterator &p_Iterator, SChunkMeta &p_ChunkMeta) const
    {
        CBBox bbox(p_Iterator->first, p_Iterator->first + CHUNK_SIZE);
        bool filtered = !m_FilterFunction;
        if(m_FilterFunction)
            filtered = m_FilterFunction(bbox, p_Iterator->second, m_Userdata);

        if(filtered)
        {
            Math::Vec3iHasher hasher;
            p_ChunkMeta = {hasher(p_Iterator->first), p_Iterator->second, bbox, p_Iterator->second->inner_bbox(p_Iterator->first)};
        }

        return filtered;
    }

    SChunkMeta CChunkQueryList::FilterNext(ankerl::unordered_dense::map<Math::Vec3i, CChunk*, Math::Vec3iHasher>::const_iterator &p_Iterator) const
    {
        while (true)
        {
            p_Iterator++;
            if(p_Iterator == m_Chunks->end())
                return SChunkMeta();

            SChunkMeta result;
            if(ApplyFilter(p_Iterator, result))
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
        if(m_Iterator != m_Parent->m_Chunks->end() && !m_Parent->ApplyFilter(m_Iterator, m_ChunkMeta))
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

    bool CChunkQueryList::CChunkQueryIterator::operator!=(const CChunkQueryIterator &p_Rhs)
    {
        return m_Iterator != p_Rhs.m_Iterator;
    }

    bool CChunkQueryList::CChunkQueryIterator::operator==(const CChunkQueryIterator &p_Rhs)
    {
        return m_Iterator == p_Rhs.m_Iterator;
    }

    CChunkQueryList::CChunkQueryIterator& CChunkQueryList::CChunkQueryIterator::operator=(const CChunkQueryIterator &p_Other)
    {
        m_ChunkMeta = p_Other.m_ChunkMeta;
        m_Parent = p_Other.m_Parent;
        m_Iterator = p_Other.m_Iterator;
        return *this;
    }

    CChunkQueryList::CChunkQueryIterator& CChunkQueryList::CChunkQueryIterator::operator=(CChunkQueryIterator &&p_Other)
    {
        m_ChunkMeta = p_Other.m_ChunkMeta;
        m_Parent = p_Other.m_Parent;
        p_Other.m_Parent = nullptr;
        m_Iterator = std::move(p_Other.m_Iterator);
        return *this;
    }

    //////////////////////////////////////////////////
    // CVoxelSpace functions
    //////////////////////////////////////////////////

    CVoxelSpace::CVoxelSpace(IStreamable *p_Stream) : m_VoxelsCount(0), m_Source(p_Stream), m_ModelLoaded(false), m_ChunkCache(Math::Vec3i(), nullptr) {}
    CVoxelSpace::CVoxelSpace(CVoxelSpace &&p_Other) : m_ChunkCache(Math::Vec3i(), nullptr) { *this = std::move(p_Other); }

    CVoxelSpace::~CVoxelSpace() 
    { 
        Clear(); 
        if(m_Source) 
            delete m_Source;
    }

    void CVoxelSpace::Insert(const pair &p_pair)
    {
        CheckLoadModel();

        Math::Vec3i position = GetChunkpos(p_pair.first);
        if(!m_ChunkCache.second || m_ChunkCache.first != position)
        {
            auto it = m_Chunks.find(position);

            // Creates a new chunk, if neccessary
            if(it == m_Chunks.end())
                it = m_Chunks.insert({position, new CChunk(this)}).first;

            m_ChunkCache = *it;
        }

        // Time to upgrade
        if(!m_ChunkCache.second->insert(p_pair)) [[unlikely]]
        {
            m_ChunkCache.second->Upgrade();
            m_ChunkCache.second->insert(p_pair);
        }

        m_VoxelsCount++;
    }

    CVoxelSpace::iterator CVoxelSpace::Erase(const iterator &p_it)
    {
        CheckLoadModel();

        Math::Vec3i position = GetChunkpos(p_it->first);
        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return end();

        auto res = it->second->erase(p_it);
        m_VoxelsCount--;

        // Removes the empty chunk.
        if(it->second->IsEmpty())
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

    CVoxelSpace::iterator CVoxelSpace::Find(const Math::Vec3i &p_v) const
    {
        const_cast<CVoxelSpace*>(this)->CheckLoadModel();
        Math::Vec3i position = GetChunkpos(p_v);
        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return end();

        CVoxel vox = it->second->find(p_v);
        if(!vox.IsInstantiated())
            return end();

        return CVoxelSpaceIterator(this, it->second->inner_bbox(it->first), {p_v, vox});
    }

    bool CVoxelSpace::HasVoxel(const Math::Vec3i &p_v) const
    {
        const_cast<CVoxelSpace*>(this)->CheckLoadModel();
        Math::Vec3i position = GetChunkpos(p_v);

        // int pos = 0;
        // for (pos = 0; pos < 9; pos++) 
        // {
        //     if(!g_Cache.Chunks[pos].second)
        //         break;

        //     if(g_Cache.Chunks[pos].first == position)
        //         return g_Cache.Chunks[pos].second;
        // }


        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return false;

        // g_Cache.Chunks[pos] = *it;

        return it->second->HasVoxel(p_v);
    }

    CVoxelSpace::querylist CVoxelSpace::QueryDirtyChunks() const
    {
        const_cast<CVoxelSpace*>(this)->CheckLoadModel();
        return CChunkQueryList(m_Chunks, [](const CBBox &p_BBox, const CChunk *p_Chunk, CChunkQueryList::IUserdata *p_Userdata)
        {
            (void)p_BBox;
            (void)p_Userdata;
            return p_Chunk->IsDirty;
        });
    }

    void CVoxelSpace::MarkAsProcessed(const SChunkMeta &p_Chunk)
    {
        auto it = m_Chunks.find(p_Chunk.TotalBBox.Beg);
        if(it != m_Chunks.end())
            it->second->IsDirty = false;
    }

    CVoxelSpace::querylist CVoxelSpace::QueryChunks() const
    {
        const_cast<CVoxelSpace*>(this)->CheckLoadModel();
        return CChunkQueryList(m_Chunks);
    }

    CVoxelSpace::querylist CVoxelSpace::QueryChunks(const CFrustum *p_Frustum, const Math::Mat4x4 &p_ModelMatrix) const
    {
        const_cast<CVoxelSpace*>(this)->CheckLoadModel();
        return CChunkQueryList(m_Chunks, [](const CBBox &p_BBox, const CChunk *p_Chunk, CChunkQueryList::IUserdata *p_Userdata)
        {
            auto data = p_Userdata->GetUserdata<FrustumQuery>();
            const CFrustum *frustum = data->Frustum;

            // Loads this chunk, if it's inside the frustum, otherwise ignore this chunk.
            if(p_Chunk->IsEmpty() && data->Source)
            {
                if(frustum->IsOnFrustum(p_BBox))
                    data->Source->ReadChunk(p_BBox.Beg, const_cast<CChunk*>(p_Chunk));
                else
                    return false;
            }
            
            return !p_Chunk->IsEmpty() && frustum->IsOnFrustum(p_Chunk->inner_bbox(data->ModelMatrix * p_BBox.Beg));
        }, new CChunkQueryList::TUserdata<FrustumQuery>(new FrustumQuery(p_Frustum, p_ModelMatrix, m_Source), true));
    }

    CVoxelSpace::iterator CVoxelSpace::Next(const Math::Vec3i &p_FromPosition) const
    {
        Math::Vec3i position = GetChunkpos(p_FromPosition);
        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return end();

        auto res = it->second->next(p_FromPosition);
        
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
        if(m_Source && !m_ModelLoaded && !m_Source->SupportsChunkOffloading())
        {
            m_ModelLoaded = true;
            m_Source->ReadVoxelSpace(*this);
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
        return CVoxelSpaceIterator(this);
    }

    CBBox CVoxelSpace::CalculateBBox() const
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

    CChunk* CVoxelSpace::CreateOrGetChunk(const Math::Vec3i &p_Position)
    {
        CheckLoadModel();

        Math::Vec3i position = GetChunkpos(p_Position);
        auto it = m_Chunks.find(position);

        // Creates a new chunk, if neccessary
        if(it == m_Chunks.end())
            it = m_Chunks.insert({position, new CChunk(this)}).first;

        return it->second;
    }

    void CVoxelSpace::Clear()
    {
        for (auto &&chunk : m_Chunks)
            delete chunk.second;
        
        m_Chunks.clear();
        m_ModelLoaded = false;
    }

    CVoxelSpace &CVoxelSpace::operator=(CVoxelSpace &&p_Other)
    {
        m_VoxelsCount = p_Other.m_VoxelsCount;
        m_Chunks = std::move(p_Other.m_Chunks);
        m_ModelLoaded = std::move(p_Other.m_ModelLoaded);

        return *this;
    }

    void CVoxelSpace::SetSource(IStreamable *p_Source)
    {
        if(m_Source)
            delete m_Source;

        m_Source = p_Source;
    }

    void CVoxelSpace::Unload()
    {
        if(m_Source && m_ModelLoaded && !m_Source->SupportsChunkOffloading())
            Clear();
    }

    CChunk *CVoxelSpace::GetChunk(const Math::Vec3i &p_Position) const
    {
        auto position = GetChunkpos(p_Position);
        auto it = m_Chunks.find(position);
        if(it == m_Chunks.end())
            return nullptr;

        return it->second;
    }
}