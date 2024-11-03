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

namespace VCore
{
    class CVoxelSpace;

    class CBitMaskChunk
    {
        public:
            CBitMaskChunk(const Math::Vec3i &_ChunkSize);
            CBitMaskChunk(CBitMaskChunk &&_Other) = default;

            void Set(const Math::Vec3i &_Position, bool _Value);
            void SetAxis(const Math::Vec3i &_Position, bool _Value, char _Axis);

            Config::bitmask_t GetRowFaces(const Math::Vec3i &_Position, char _Axis) const;

            CBitMaskChunk &operator=(CBitMaskChunk &&_Other) = default;
            CBitMaskChunk &operator=(const CBitMaskChunk &_Other) = delete;

        private:
            std::vector<Config::bitmask_t> m_Grid;
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
            void insert(CVoxelSpace *_Space, const pair &_pair);

            /**
             * @brief Removes a voxel.
             */
            ppair erase(CVoxelSpace *_Space, const iterator &_it);

            /**
             * @brief Returns the next voxel or null.
             */
            ppair next(const Math::Vec3i &_Position) const;

            /**
             * @brief Tries to find a voxel.
             * @brief Returns a reference to the voxel.
             */
            Voxel find(const Math::Vec3i &_v) const;

            inline CBBox inner_bbox(const Math::Vec3i &_Position) const
            {
                return CBBox(m_InnerBBox.Beg + _Position, m_InnerBBox.End + _Position);
            }

            CChunk &operator=(CChunk &&_Other);
            CChunk &operator=(const CChunk &_Other) = delete;

            ~CChunk() { clear(); }


            CBitMaskChunk m_Mask;

        private:
            // CVoxel *GetBlock(CVoxelSpace *_Space, const CBBox &_ChunkDim, const Math::Vec3i &_v);
            bool HasVoxelOnPlane(int _Axis, const Math::Vec3i &_Pos);

            void clear();

            CVoxel *m_Data;
            CBBox m_InnerBBox;
    };

    struct SChunkMeta
    {
        size_t UniqueId;            //!< Unique identifier of the chunks. Only changes, if the voxel mesh is resized.
        const CChunk *Chunk;        //!< Chunk with is associated with this metadata.
        CBBox TotalBBox;            //!< The total bounding box of the chunk.
        CBBox InnerBBox;            //!< The bounding box of the model inside the chunk.
    };

    inline Math::Vec3i GetChunkpos(const Math::Vec3i &_Position)
    {
        return _Position & Config::ChunkPositionMask;
    }
} // namespace VCore


#endif