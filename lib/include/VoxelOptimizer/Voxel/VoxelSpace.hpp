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

#include <VoxelOptimizer/Math/Vector.hpp>
#include <VoxelOptimizer/Voxel/BBox.hpp>
#include <VoxelOptimizer/Voxel/Voxel.hpp>
#include <VoxelOptimizer/Voxel/Frustum.hpp>

#include <vector>

namespace VoxelOptimizer
{
    class CVoxelSpace;
    class CChunk;

    struct SChunkMeta
    {
        size_t UniqueId;    //!< Unique identifier of the chunks. Only changes, if the voxel mesh is resized.
        const CChunk *Chunk;      //!< Chunk with is associated with this metadata.
        CBBox TotalBBox;    //!< The total bounding box of the chunk.
        CBBox InnerBBox;    //!< The bounding box of the model inside the chunk.
    };

    class CVoxelSpaceIterator
    {
        public:
            using pair = std::pair<Math::Vec3i, Voxel>;
            using reference = std::pair<Math::Vec3i, Voxel>&;
            using pointer = pair*;

            CVoxelSpaceIterator();
            CVoxelSpaceIterator(const CVoxelSpace *_Space, const CBBox &_InnerBox, const pair &_Pair);
            CVoxelSpaceIterator(const CVoxelSpaceIterator &_Other);
            CVoxelSpaceIterator(CVoxelSpaceIterator &&_Other);

            const reference operator*() const;
            const pointer operator->() const;

            CVoxelSpaceIterator& operator++();
            CVoxelSpaceIterator& operator++(int);

            bool operator!=(const CVoxelSpaceIterator &_Rhs);
            bool operator==(const CVoxelSpaceIterator &_Rhs);

            CVoxelSpaceIterator& operator=(const CVoxelSpaceIterator &_Other);
            CVoxelSpaceIterator& operator=(CVoxelSpaceIterator &&_Other);
        
        private:
            const CVoxelSpace *m_Space;
            CBBox m_InnerBox;
            mutable pair m_Pair;
    };

    class CChunk
    {
        public:
            using ppair = std::pair<Math::Vec3i, Voxel>;
            using pair = std::pair<Math::Vec3i, CVoxel>;
            using iterator = CVoxelSpaceIterator;

            bool IsDirty;

            CChunk() = delete;
            CChunk(const CChunk &_Other) = delete;
            CChunk(const Math::Vec3i &_ChunkSize);
            CChunk(CChunk &&_Other);

            /**
             * @brief Insert a new voxel.
             */
            void insert(const pair &_pair, const CBBox &_ChunkDim);

            /**
             * @brief Removes a voxel.
             */
            ppair erase(const iterator &_it, const CBBox &_ChunkDim);

            /**
             * @brief Returns the next voxel or null.
             */
            ppair next(const Math::Vec3i &_Position, const CBBox &_ChunkDim) const;

            /**
             * @brief Tries to find a voxel.
             * @brief Returns a reference to the voxel.
             */
            Voxel find(const Math::Vec3i &_v, const CBBox &_ChunkDim) const;

            /**
             * @brief Tries to find a voxel.
             * @param _Opaque: If true only opaque voxels are returned, otherwise only none opaque voxels are returned.
             * @brief Returns a reference to the voxel.
             */
            Voxel find(const Math::Vec3i &_v, const CBBox &_ChunkDim, bool _Opaque) const;

            /**
             * @brief Same as find. but only for visible voxels.
             */
            Voxel findVisible(const Math::Vec3i &_v, const CBBox &_ChunkDim) const;

            /**
             * @brief Same as find. but only for visible voxels.
             */
            Voxel findVisible(const Math::Vec3i &_v, const CBBox &_ChunkDim, bool _Opaque) const;

            inline CBBox inner_bbox(const Math::Vec3i &_Position) const
            {
                return CBBox(m_InnerBBox.Beg + _Position, m_InnerBBox.End + _Position);
            }

            CChunk &operator=(CChunk &&_Other);
            CChunk &operator=(const CChunk &_Other) = delete;

            ~CChunk() { clear(); }

        private:
            void clear();

            CVoxel *m_Data;
            CBBox m_InnerBBox;
    };

    class CVoxelSpace
    {
        friend CVoxelSpaceIterator;

        public:
            using ppair = std::pair<Math::Vec3i, Voxel>;
            using pair = std::pair<Math::Vec3i, CVoxel>;
            using iterator = CVoxelSpaceIterator;

            CVoxelSpace();
            CVoxelSpace(const Math::Vec3i &_ChunkSize);
            CVoxelSpace(const CVoxelSpace &_Other) = delete;;
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
             * @brief Tries to find a voxel.
             * @param _Opaque: If true only opaque voxels are returned, otherwise only none opaque voxels are returned.
             * @return Returns an iterator to the voxel or ::end()
             */
            iterator find(const Math::Vec3i &_v, bool _Opaque) const;

            /**
             * @brief Same as find. but only for visible voxels.
             */
            iterator findVisible(const Math::Vec3i &_v) const;

            /**
             * @brief Same as find. but only for visible voxels.
             */
            iterator findVisible(const Math::Vec3i &_v, bool _Opaque) const;

            /**
             * @brief Queries all visible voxels.
             * @param opaque: If true only opaque voxels are returned, otherwise only none opaque voxels are returned.
             */
            VectoriMap<Voxel> queryVisible(bool opaque) const;

            /**
             * @return Gets a list of all chunks which has been modified.
             * @note Marks all chunks as processed.
             */
            std::vector<SChunkMeta> queryDirtyChunks(const CFrustum *_Frustum = nullptr) const;

            /**
             * @brief Marks a dirty chunks as clean.
             */
            void markAsProcessed(const SChunkMeta &_Chunk);

            /**
             * @return Returns all chunks.
             */
            std::vector<SChunkMeta> queryChunks(const CFrustum *_Frustum = nullptr) const;

            /**
             * @brief Generates and updates all the visibility masks of the voxels.
             */
            void generateVisibilityMask();

            /**
             * @brief Updates the visibility mask of a given voxel and its neighbours.
             */
            void updateVisibility(const Math::Vec3i &_Position);

            /**
             * @return Gets the voxel count.
             */
            inline size_t size() const
            {
                return m_VoxelsCount;
            }

            iterator begin();
            iterator end() const;

            void clear();

            CVoxelSpace &operator=(const CVoxelSpace &_Other) = delete;;
            CVoxelSpace &operator=(CVoxelSpace &&_Other);

            ~CVoxelSpace() { clear(); }

        private:
            Math::Vec3i chunkpos(const Math::Vec3i &_Position) const;
            void CheckVisibility(const Voxel &_v, const Voxel &_v2, char _axis);

            iterator next(const Math::Vec3i &_FromPosition) const;

            Math::Vec3i m_Size;
            Math::Vec3i m_ChunkSize;
            size_t m_VoxelsCount;
            std::unordered_map<Math::Vec3i, CChunk, Math::Vec3iHasher> m_Chunks;
    };
}


#endif