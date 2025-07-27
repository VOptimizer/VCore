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

#ifndef FACEMASK_HPP
#define FACEMASK_HPP

#include <cstddef>
#include <VCore/Voxel/VoxelModel.hpp>
#include <VCore/Misc/fast_vector.hpp>

namespace VCore
{
    class CFaceMask
    {
        public:
            struct Mask
            {
                Config::bitmask_t Bits[(Config::ChunkSize + 2) * 2];
            };

            CFaceMask() = default;

            bool GroupAfterMaterial = false;

            /**
             * @brief Generates the face bit mask for the given chunk on the axis.
             */
            ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<uint32_t, Mask>> Generate(const VoxelModel &p_Model, const SChunkMeta &p_Chunk, const uint8_t p_Axis);

            ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<uint32_t, Mask>> Generate(const VoxelModel &p_Model, Math::Vec3i p_ChunkPos, const uint8_t p_Axis);

            ~CFaceMask() = default;

        private:
            struct OpaqueMask
            {
                Config::bitmask_t Opaque = 0;
                Config::bitmask_t Transparent = 0;
            };

            void InternalGenerate();
            void FillVoxelBits(Config::bitmask_t *p_opaqueVoxels, Config::bitmask_t *p_transparentVoxels, const CChunk *p_Chunk, const Math::Vec3i &p_Position, const int p_Count);
            void GenerateMask(Config::bitmask_t *p_Voxels, const Math::Vec3i &p_Subpos, const int p_Count);
            void FillSlice(Config::bitmask_t p_Faces, const Math::Vec3i &p_Subpos, const int p_Column, const bool p_Backface, ankerl::unordered_dense::map<uint32_t, Mask> &p_Masks);

            VoxelModel m_Model;
            SChunkMeta m_Chunk;

            // X = Run axis, Y = Height axis, Z = Width axis.
            Math::TVector3<uint8_t> m_Axis;

            fast_vector<int> m_TransparentMaterials;
            Mask *m_MaskCache;
            uint32_t m_CachedKey;

            ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<uint32_t, Mask>> m_FacesMasks;
    };
} // namespace VCore


#endif