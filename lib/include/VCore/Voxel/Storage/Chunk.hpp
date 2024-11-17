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

#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "Iterator.hpp"
#include <VCore/Voxel/BBox.hpp>
#include "../Voxel.hpp"
#include <utility>
#include <VCore/VConfig.hpp>
#include <VCore/Memory/MemoryPool.hpp>

namespace VCore
{
    class CVoxelSpace;

    class CBitMaskChunk
    {
        public:
            CBitMaskChunk() = default;

            void Set(const Math::Vec3i &_Position, bool _Value);
            void SetAxis(const Math::Vec3i &_Position, bool _Value, char _Axis);

            CBitMaskChunk &operator=(const CBitMaskChunk &_Other);

            Config::bitmask_t GetRowFaces(const Math::Vec3i &_Position, char _Axis) const;
        private:
            Config::bitmask_t m_Grid[Config::ChunkSize * Config::ChunkSize * 3] = {};
    };

    class IChunk
    {
        public:
            using pair = std::pair<Math::Vec3i, CVoxel>;
            using iterator = CVoxelSpaceIterator;

            bool IsDirty;
            CBitMaskChunk m_Mask;

            IChunk() : IsDirty(false), m_InnerBBox(Math::Vec3i(INT32_MAX, INT32_MAX, INT32_MAX), Math::Vec3i()) { }

            /**
             * @brief Upgrades this chunk to the next bigger one.
             * @return Returns the new chunk instance or null, if a upgrade is not possible.
             */
            IChunk *Upgrade();

            /**
             * @brief Insert a new voxel.
             * @return Returns true on success and false, if the chunk needs to be upgraded.
             */
            bool insert(CVoxelSpace *_Space, const pair &_pair);

            /**
             * @brief Removes a voxel.
             */
            pair erase(CVoxelSpace *_Space, const iterator &_it);

            /**
             * @brief Returns the next voxel or null.
             */
            pair next(const Math::Vec3i &_Position) const;

            /**
             * @brief Tries to find a voxel.
             * @brief Returns a reference to the voxel.
             */
            CVoxel find(const Math::Vec3i &_v) const;

            inline CBBox inner_bbox(const Math::Vec3i &_Position) const
            {
                return CBBox(m_InnerBBox.Beg + _Position, m_InnerBBox.End + _Position);
            }

            virtual ~IChunk() = default;

        protected:
            /** Sets a voxel on a given position. */
            virtual bool SetVoxel(const CVoxel &_Voxel, const Math::Vec3i &_Position) = 0;

            /**
             * @return Returns either an instantiated or not instantiated voxel.
             */
            virtual CVoxel GetVoxel(const Math::Vec3i &_Position) const = 0;

            CBBox m_InnerBBox;
        private:
            bool HasVoxelOnPlane(int _Axis, const Math::Vec3i &_Pos);

            void UpdateNeighborChunks(CVoxelSpace *_Space, bool _Value, const Math::Vec3i &_GlobalPos);
    };

    class CByteChunk : public IChunk
    {
        public:
            CByteChunk() : m_VoxelIndexSize(0) 
            {
                Clear();
            }

            void *operator new(size_t n)
            {
                // 59424
                return m_Pool.allocate(n);
            }

            void operator delete(void *p)
            {
                m_Pool.deallocate((CByteChunk*)p, sizeof(CByteChunk));
            }

            virtual ~CByteChunk() { Clear(); }

        private:
            static CMemoryPool<CByteChunk> m_Pool;
            static constexpr int HASHMAP_SIZE = 254;

            struct VoxelRef
            {
                VoxelRef() : RefCount(0) {}

                CVoxel Voxel;
                uint32_t RefCount;
            };
            
            VoxelRef m_VoxelIndex[HASHMAP_SIZE] = {};
            uint8_t m_VoxelIndexSize;
            uint8_t m_Data[Config::ChunkSize * Config::ChunkSize * Config::ChunkSize] = {};

            uint8_t AddAndGetVoxelIndex(const CVoxel &_Voxel);
            void Clear();

        protected:
            bool SetVoxel(const CVoxel &_Voxel, const Math::Vec3i &_Position) override;
            CVoxel GetVoxel(const Math::Vec3i &_Position) const override;
    };

    class CChunk : public IChunk
    {
        public:
            void *operator new(size_t n)
            {
                // 155696
                return m_Pool.allocate(n);
            }

            void operator delete(void *p)
            {
                m_Pool.deallocate((CChunk*)p, sizeof(CChunk));
            }

            virtual ~CChunk() { Clear(); }

        protected:
            bool SetVoxel(const CVoxel &_Voxel, const Math::Vec3i &_Position) override;
            CVoxel GetVoxel(const Math::Vec3i &_Position) const override;

        private:
            static CMemoryPool<CChunk> m_Pool;

            void Clear();
            CVoxel m_Data[Config::ChunkSize * Config::ChunkSize * Config::ChunkSize];
    };

    struct SChunkMeta
    {
        size_t UniqueId;            //!< Unique identifier of the chunks. Only changes, if the voxel mesh is resized.
        const IChunk *Chunk;        //!< Chunk with is associated with this metadata.
        CBBox TotalBBox;            //!< The total bounding box of the chunk.
        CBBox InnerBBox;            //!< The bounding box of the model inside the chunk.
    };

    inline Math::Vec3i GetChunkpos(const Math::Vec3i &_Position)
    {
        return _Position & Config::ChunkPositionMask;
    }
} // namespace VCore


#endif