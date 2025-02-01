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

#include "Chunk.hpp"

#include <vector>

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
                CChunkQueryIterator(const CChunkQueryList *_Parent, ankerl::unordered_dense::map<Math::Vec3i, IChunk*, Math::Vec3iHasher>::const_iterator _Iterator) : m_Parent(_Parent), m_Iterator(_Iterator) {}
                CChunkQueryIterator(CChunkQueryIterator &&_Other) { *this = std::move(_Other); }
                CChunkQueryIterator(const CChunkQueryIterator &_Other) { *this = _Other; }

                reference operator*() const;
                pointer operator->() const;

                CChunkQueryIterator& operator++();
                CChunkQueryIterator& operator++(int);

                bool operator!=(const CChunkQueryIterator &_Rhs);
                bool operator==(const CChunkQueryIterator &_Rhs);

                CChunkQueryIterator& operator=(const CChunkQueryIterator &_Other);
                CChunkQueryIterator& operator=(CChunkQueryIterator &&_Other);
            private:
                void InitFilter();

                const CChunkQueryList *m_Parent; 
                mutable SChunkMeta m_ChunkMeta;

                ankerl::unordered_dense::map<Math::Vec3i, IChunk*, Math::Vec3iHasher>::const_iterator m_Iterator;
        };

        public:
            using iterator = CChunkQueryIterator;
            using FilterFunction = bool (*)(const CBBox &_BBox, const IChunk *_Chunk, void *_Userdata);

            CChunkQueryList() : m_FilterFunction(nullptr), m_Chunks(nullptr) {}
            CChunkQueryList(const ankerl::unordered_dense::map<Math::Vec3i, IChunk*, Math::Vec3iHasher> &_Chunks, FilterFunction _FilterFn = nullptr, void *_Userdata = nullptr) : m_FilterFunction(_FilterFn), m_Chunks(&_Chunks), m_Userdata(_Userdata) {}
            CChunkQueryList(const CChunkQueryList &_Other) { *this = _Other; }
            CChunkQueryList(CChunkQueryList &&_Other) { *this = std::move(_Other); }

            iterator begin();
            iterator end();

            iterator begin() const;
            iterator end() const;

            operator std::vector<SChunkMeta>() const;

            CChunkQueryList &operator=(const CChunkQueryList &_Other);
            CChunkQueryList &operator=(CChunkQueryList &&_Other);

        private:
            bool ApplyFilter(ankerl::unordered_dense::map<Math::Vec3i, IChunk*, Math::Vec3iHasher>::const_iterator &_Iterator, SChunkMeta &_ChunkMeta) const;
            SChunkMeta FilterNext(ankerl::unordered_dense::map<Math::Vec3i, IChunk*, Math::Vec3iHasher>::const_iterator &_Iterator) const;

            FilterFunction m_FilterFunction;
            const ankerl::unordered_dense::map<Math::Vec3i, IChunk*, Math::Vec3iHasher> *m_Chunks;
            void *m_Userdata;
    };

    class CVoxelSpace
    {
        friend CVoxelSpaceIterator;
        friend IChunk;

        public:
            using pair = IChunk::pair; // std::pair<Math::Vec3i, CVoxel>;
            using iterator = CVoxelSpaceIterator;
            using querylist = CChunkQueryList;

            ankerl::unordered_dense::map<TextureType, Texture> Textures;   //!< Used colors
            std::string Name; //!< Name of this model.

            CVoxelSpace(IStreamable *_Stream = nullptr);
            CVoxelSpace(const CVoxelSpace &_Other) = delete;
            CVoxelSpace(CVoxelSpace &&_Other);

            /**
             * @brief Insert a new voxel.
             */
            void insert(const pair &_pair);

            /**
             * @brief Removes a voxel.
             */
            iterator erase(const iterator &_it);

            /**
             * @brief Tries to find a voxel.
             * @return Returns an iterator to the voxel or ::end()
             */
            iterator find(const Math::Vec3i &_v) const;

            /**
             * @return Gets a list of all chunks which has been modified.
             * @note Marks all chunks as processed.
             */
            querylist queryDirtyChunks() const;

            /**
             * @brief Marks a dirty chunks as clean.
             */
            void markAsProcessed(const SChunkMeta &_Chunk);

            /**
             * @return Returns all chunks.
             */
            querylist queryChunks() const;

            /**
             * @return Returns a list of all chunks which are falling inside the given frustum.
             */
            querylist queryChunks(const CFrustum *_Frustum) const;

            /**
             * @return Gets the voxel count.
             */
            inline size_t size() const { return m_VoxelsCount; }

            iterator begin();
            iterator end() const;

            CBBox calculateBBox() const;

            inline const IChunk *getChunk(const Math::Vec3i &_Position)
            {
                return GetChunk(_Position);
            }

            void clear();

            CVoxelSpace &operator=(const CVoxelSpace &_Other) = delete;
            CVoxelSpace &operator=(CVoxelSpace &&_Other);

            ~CVoxelSpace();

        private:
            IChunk *GetChunk(const Math::Vec3i &_Position);
            iterator next(const Math::Vec3i &_FromPosition) const;

            // Checks if the whole model needs to be loaded.
            void CheckLoadModel();

            size_t m_VoxelsCount;
            ankerl::unordered_dense::map<Math::Vec3i, IChunk*, Math::Vec3iHasher> m_Chunks;

            // Allows to stream content from and to disk or other storage devices.
            IStreamable *m_Stream;
            bool m_ModelLoaded;
    };

    using VoxelModel = std::shared_ptr<CVoxelSpace>;
}


#endif