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
#include <cstdint>
#include <utility>
#include <VCore/VConfig.hpp>

namespace VCore
{
    class CVoxelSpace;

    // Stores voxels as bitmask in a column first manner for all axis.
    class CBitMaskChunk
    {
        public:
            CBitMaskChunk() = default;

            void Set(const Math::Vec3i &p_Position, bool p_Value);
            void SetAxis(const Math::Vec3i &p_Position, bool p_Value, char p_Axis);

            CBitMaskChunk &operator=(const CBitMaskChunk &p_Other);

            Config::bitmask_t GetRowFaces(const Math::Vec3i &p_Position, char p_Axis) const;
        private:
            Config::bitmask_t m_Grid[Config::ChunkSize * Config::ChunkSize * 3] = {};
    };

    class IChunkStorage
    {
        public:
            /** Sets a voxel on a given position. */
            virtual bool SetVoxel(const CVoxel &p_Voxel, const Math::Vec3i &p_Position) = 0;

            /** 
             * @return Returns either an instantiated or not instantiated voxel. 
             */
            virtual CVoxel GetVoxel(const Math::Vec3i &p_Position) const = 0;

            virtual ~IChunkStorage() = default;
    };

    class CChunk
    {
        public:
            using pair = std::pair<Math::Vec3i, CVoxel>;
            using iterator = CVoxelSpaceIterator;

            bool IsDirty;
            CBitMaskChunk Mask;

            CChunk(CVoxelSpace *p_Space) : IsDirty(false), m_Space(p_Space), m_Storage(nullptr), m_InnerBBox(Math::Vec3i(INT32_MAX, INT32_MAX, INT32_MAX), Math::Vec3i()) { }
            

            /**
             * @brief Upgrades this chunk to the next bigger one.
             * @return Returns the new chunk instance or null, if a upgrade is not possible.
             */
            void Upgrade();

            /**
             * @brief Insert a new voxel.
             * @return Returns true on success and false, if the chunk needs to be upgraded.
             */
            bool insert(const pair &p_pair);

            /**
             * @brief Removes a voxel.
             */
            pair erase(const iterator &p_it);

            /**
             * @brief Returns the next voxel or null.
             */
            pair next(const Math::Vec3i &p_Position) const;

            /**
             * @brief Tries to find a voxel.
             * @brief Returns a reference to the voxel.
             */
            CVoxel find(const Math::Vec3i &p_v) const;

            bool HasVoxel(const Math::Vec3i &p_v) const;

            inline CBBox inner_bbox(const Math::Vec3i &p_Position) const
            {
                return CBBox(m_InnerBBox.Beg + p_Position, m_InnerBBox.End + p_Position);
            }

            inline bool IsEmpty() const { return m_Storage == nullptr; }

            virtual ~CChunk() = default;

        protected:
            CVoxelSpace *m_Space;
            IChunkStorage *m_Storage;
            CBBox m_InnerBBox;
        
        private:
            bool HasVoxelOnPlane(int p_Axis, const Math::Vec3i &p_Pos);
            void UpdateNeighborChunks(CVoxelSpace *p_Space, const Math::Vec3i &p_GlobalPos);
    };

    // class CUMapChunk : public IChunkStorage
    // {
    //     public:
    //         bool SetVoxel(const CVoxel &p_Voxel, const Math::Vec3i &p_Position) override;
    //         CVoxel GetVoxel(const Math::Vec3i &p_Position) const override;

    //         virtual ~CUMapChunk() { Clear(); }

    //     private:
    //         void Clear();

    //         ankerl::unordered_dense::map<uint32_t, CVoxel> m_Data;
    //         // CVoxel m_Data[Config::ChunkSize * Config::ChunkSize * Config::ChunkSize];
    // };

    class CByteChunk : public IChunkStorage
    {
        public:
            CByteChunk() : m_VoxelIndexSize(0) { Clear(); }

            bool SetVoxel(const CVoxel &p_Voxel, const Math::Vec3i &p_Position) override;
            CVoxel GetVoxel(const Math::Vec3i &p_Position) const override;

            virtual ~CByteChunk() { Clear(); }
        private:
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

            uint8_t AddAndGetVoxelIndex(const CVoxel &p_Voxel);
            void Clear();
    };

    class CChunk32 : public IChunkStorage
    {
        public:
            bool SetVoxel(const CVoxel &p_Voxel, const Math::Vec3i &p_Position) override;
            CVoxel GetVoxel(const Math::Vec3i &p_Position) const override;

            virtual ~CChunk32() { Clear(); }

        private:
            void Clear();
            CVoxel m_Data[Config::ChunkSize * Config::ChunkSize * Config::ChunkSize];
    };

    struct SChunkMeta
    {
        size_t UniqueId;            //!< Unique identifier of the chunks.
        const CChunk *Chunk;        //!< Chunk with is associated with this metadata.
        CBBox TotalBBox;            //!< The total bounding box of the chunk.
        CBBox InnerBBox;            //!< The bounding box of the model inside the chunk.
    };

    inline Math::Vec3i GetChunkpos(const Math::Vec3i &p_Position)
    {
        return p_Position & Config::ChunkPositionMask;
    }
} // namespace VCore


#endif