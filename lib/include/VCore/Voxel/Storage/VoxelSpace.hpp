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

#ifndef VOXELSPACE_HPP
#define VOXELSPACE_HPP

#include <VCore/Math/Vector.hpp>
#include <VCore/Voxel/BBox.hpp>
#include <VCore/Voxel/Voxel.hpp>
#include <VCore/Voxel/Frustum.hpp>
#include <VCore/VConfig.hpp>
#include <VCore/Meshing/Texture.hpp>

#include <VCore/Voxel/Storage/Chunk.hpp>
#include <VCore/Math/Mat4x4.hpp>
#include <VCore/Misc/MessageBus.hpp>
#include <cstdint>

namespace VCore
{
    class CVoxelSpace;
    class IStreamable;

    class CChunkQueryList
    {
        class CChunkQueryIterator
        {
            friend CChunkQueryList;
            public:
                using reference = SChunkMeta&;
                using pointer = SChunkMeta*;

                CChunkQueryIterator() : m_Parent(nullptr) {}
                CChunkQueryIterator(const CChunkQueryList *p_Parent, ankerl::unordered_dense::map<Math::Vec3i, CChunk*, Math::Vec3iHasher>::const_iterator p_Iterator) : m_Parent(p_Parent), m_Iterator(p_Iterator) {}
                CChunkQueryIterator(CChunkQueryIterator &&p_Other) { *this = std::move(p_Other); }
                CChunkQueryIterator(const CChunkQueryIterator &p_Other) { *this = p_Other; }

                reference operator*() const;
                pointer operator->() const;

                CChunkQueryIterator& operator++();
                CChunkQueryIterator& operator++(int);

                bool operator!=(const CChunkQueryIterator &p_Rhs);
                bool operator==(const CChunkQueryIterator &p_Rhs);

                CChunkQueryIterator& operator=(const CChunkQueryIterator &p_Other);
                CChunkQueryIterator& operator=(CChunkQueryIterator &&p_Other);
            private:
                void InitFilter();

                const CChunkQueryList *m_Parent; 
                mutable SChunkMeta m_ChunkMeta;

                ankerl::unordered_dense::map<Math::Vec3i, CChunk*, Math::Vec3iHasher>::const_iterator m_Iterator;
        };

        public:
            class IUserdata
            {
                public:
                    IUserdata(bool p_UserdataOwner) : m_UserdataOwner(p_UserdataOwner) {}

                    template<class T>
                    T *GetUserdata() const
                    {
                        return static_cast<T*>(GetUserdataInternal());
                    }

                    virtual ~IUserdata() = default;
                protected:
                    virtual void *GetUserdataInternal() const = 0;

                    bool m_UserdataOwner;
            };

            template<class T>
            class TUserdata : public IUserdata
            {
                public:
                    TUserdata(T* p_Userdata, bool p_UserdataOwner) : IUserdata(p_UserdataOwner), m_Userdata(p_Userdata) {}

                    virtual ~TUserdata()
                    {
                        if(m_UserdataOwner && m_Userdata)
                        {
                            delete m_Userdata;
                            m_Userdata = nullptr;
                        }
                    }

                protected:
                    void *GetUserdataInternal() const override
                    {
                        return m_Userdata;
                    }

                private:
                    T* m_Userdata;
            };

            using iterator = CChunkQueryIterator;
            using FilterFunction = bool (*)(const CBBox &p_BBox, const CChunk *p_Chunk, IUserdata *p_Userdata);

            CChunkQueryList() : m_FilterFunction(nullptr), m_Chunks(nullptr) {}
            CChunkQueryList(const ankerl::unordered_dense::map<Math::Vec3i, CChunk*, Math::Vec3iHasher> &p_Chunks, FilterFunction p_FilterFn = nullptr, IUserdata *p_Userdata = nullptr) : m_FilterFunction(p_FilterFn), m_Chunks(&p_Chunks), m_Userdata(p_Userdata) {}
            CChunkQueryList(const CChunkQueryList &p_Other) { *this = p_Other; }
            CChunkQueryList(CChunkQueryList &&p_Other) { *this = std::move(p_Other); }

            iterator begin();
            iterator end();

            iterator begin() const;
            iterator end() const;

            operator std::vector<SChunkMeta>() const;

            CChunkQueryList &operator=(const CChunkQueryList &p_Other);
            CChunkQueryList &operator=(CChunkQueryList &&p_Other);

            ~CChunkQueryList()
            {
                if(m_Userdata)
                    delete m_Userdata;
            }
        private:
            bool ApplyFilter(ankerl::unordered_dense::map<Math::Vec3i, CChunk*, Math::Vec3iHasher>::const_iterator &p_Iterator, SChunkMeta &p_ChunkMeta) const;
            SChunkMeta FilterNext(ankerl::unordered_dense::map<Math::Vec3i, CChunk*, Math::Vec3iHasher>::const_iterator &p_Iterator) const;

            FilterFunction m_FilterFunction;
            const ankerl::unordered_dense::map<Math::Vec3i, CChunk*, Math::Vec3iHasher> *m_Chunks;
            IUserdata *m_Userdata;
    };

    class CVoxelSpace
    {
        friend CVoxelSpaceIterator;
        friend CChunk;

        public:
            using pair = CChunk::pair; // std::pair<Math::Vec3i, CVoxel>;
            using iterator = CVoxelSpaceIterator;
            using querylist = CChunkQueryList;

            Math::Vec3f Origin;

            CVoxelSpace(IStreamable *p_Stream = nullptr);
            CVoxelSpace(const CVoxelSpace &p_Other) = delete;
            CVoxelSpace(CVoxelSpace &&p_Other);

            /**
             * @brief Insert a new voxel.
             */
            void Insert(const pair &p_pair);

            /**
             * @brief Removes a voxel.
             */
            iterator Erase(const iterator &p_it);

            /**
             * @brief Tries to find a voxel.
             * @return Returns an iterator to the voxel or ::end()
             */
            iterator Find(const Math::Vec3i &p_v) const;

            /**
             * @return Gets a list of all chunks which has been modified.
             * @note Marks all chunks as processed.
             */
            querylist QueryDirtyChunks() const;

            /**
             * @brief Marks a dirty chunks as clean.
             */
            void MarkAsProcessed(const SChunkMeta &p_Chunk);

            /**
             * @return Returns all chunks.
             */
            querylist QueryChunks() const;

            /**
             * @return Returns a list of all chunks which are falling inside the given frustum.
             */
            querylist QueryChunks(const CFrustum *p_Frustum, const Math::Mat4x4 &p_ModelMatrix = Math::Mat4x4()) const;

            void NotifyChanged() { ModelBBoxMessageBus::GetInstance()->PublishMessage((uintptr_t)this, CalculateBBox()); }

            /**
             * @return Gets the voxel count.
             */
            inline size_t Size() const { return m_VoxelsCount; }

            iterator begin();
            iterator end() const;

            CBBox CalculateBBox() const;

            CChunk* CreateOrGetChunk(const Math::Vec3i &p_Position);

            /** Gets the chunk at the given position or null, if not found */
            CChunk *GetChunk(const Math::Vec3i &p_Position) const;
            
            /** Deletes all voxel data. */
            void Clear();

            CVoxelSpace &operator=(const CVoxelSpace &p_Other) = delete;
            CVoxelSpace &operator=(CVoxelSpace &&p_Other);

            void SetSource(IStreamable *p_Source);

            /** Unloads this model. */
            void Unload();

            ~CVoxelSpace();

        private:
            iterator Next(const Math::Vec3i &p_FromPosition) const;

            // Checks if the whole model needs to be loaded.
            void CheckLoadModel();

            size_t m_VoxelsCount;
            ankerl::unordered_dense::map<Math::Vec3i, CChunk*, Math::Vec3iHasher> m_Chunks;

            // Allows to stream content from and to disk or other storage devices.
            mutable IStreamable *m_Source;
            bool m_ModelLoaded;
            std::pair<Math::Vec3i, CChunk*> m_ChunkCache;
    };

    using VoxelModel = std::shared_ptr<CVoxelSpace>;
}


#endif