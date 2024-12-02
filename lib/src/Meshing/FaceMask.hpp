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

#include <VCore/Voxel/VoxelModel.hpp>
#include <VCore/Misc/fast_vector.hpp>
#include <map>

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
            ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<uint32_t, Mask>> Generate(const VoxelModel &_Model, const SChunkMeta &_Chunk, const uint8_t _Axis);

            ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<uint32_t, Mask>> Generate(const VoxelModel &_Model, Math::Vec3i _ChunkPos, const uint8_t _Axis);

            ~CFaceMask() = default;

        private:
            struct OpaqueMask
            {
                Config::bitmask_t Opaque = 0;
                Config::bitmask_t Transparent = 0;
            };

            void InternalGenerate(int _ChunkMask);
            void FillVoxelBits(int *_opaqueVoxels, int *_transparentVoxels, const IChunk *_Chunk, const Math::Vec3i &_Position, const int _Count);
            void GenerateMask(int *_Voxels, const Math::Vec3i &_Subpos, const int _Count);
            void FillSlice(uint32_t _Faces, const Math::Vec3i &_Subpos, const int _Column, const bool _Backface, ankerl::unordered_dense::map<uint32_t, Mask> &_Masks);

            VoxelModel m_Model;
            SChunkMeta m_Chunk;

            // X = Run axis, Y = Height axis, Z = Width axis.
            Math::TVector3<char> m_Axis;

            fast_vector<int> m_TransparentMaterials;
            Mask *m_MaskCache;
            uint32_t m_CachedKey;

            ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<uint32_t, Mask>> m_FacesMasks;
    };
} // namespace VCore


#endif