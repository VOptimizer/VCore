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
#include <map>

namespace VCore
{
    class CFaceMask
    {
        public:
            struct Mask
            {
                BITMASK_TYPE Bits[CHUNK_SIZE * 2];
            };

            CFaceMask() = default;

            /**
             * @brief Generates the face bit mask for the given chunk on the axis.
             */
            ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<std::string, Mask>> Generate(const VoxelModel &_Model, const SChunkMeta &_Chunk, uint8_t _Axis);

            ~CFaceMask() = default;

        private:
            struct OpaqueMask
            {
                BITMASK_TYPE Opaque = 0;
                BITMASK_TYPE Transparent = 0;
            };

            void GenerateMask(BITMASK_TYPE faces, bool backFace, Math::Vec3i position, const Math::Vec3i &_Axis, const SChunkMeta &_Chunk);
            OpaqueMask GenerateOpaqueMask(const VoxelModel &_Model, const SChunkMeta &_Chunk, BITMASK_TYPE _Voxels, Math::Vec3i position, uint8_t _Axis);
            
            ankerl::unordered_dense::map<int, ankerl::unordered_dense::map<std::string, Mask>> m_FacesMasks;
    };
} // namespace VCore


#endif