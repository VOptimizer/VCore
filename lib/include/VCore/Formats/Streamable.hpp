/*
 * MIT License
 *
 * Copyright (c) 2025 Christian Tost
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

#include <VCore/Voxel/Storage/VoxelSpace.hpp>
#include <VCore/Misc/fast_vector.hpp>

namespace VCore
{
    /**
     * Allows models to offload on disk or other storage devices.
     */
    class IStreamable
    {
        public:
            IStreamable() : m_IsReadOnly(true) {}

            /** @return Returns true, if this stream has support for reading and writing individual chunks */
            virtual bool SupportsChunkOffloading() const = 0;

            /** @return Returns true, if this stream is readonly. */
            virtual bool IsReadOnly() const { return m_IsReadOnly; };

            /** 
             * @brief Writes a chunk to the stream. 
             * 
             * @param _Position Position of the chunk
             * @param _Chunk Chunk which should be stored.
             * 
             * @return Returns true on success otherwise false.
             */
            virtual bool WriteChunk(const Math::Vec3i &_Position, const IChunk *_Chunk) { return false; }

            /** 
             * @brief Reads a chunk from the stream. 
             * 
             * @param _Position Position of the chunk
             * 
             * @return Returns a new chunk or null on error.
             */
            virtual IChunk* ReadChunk(const Math::Vec3i &_Position) { return nullptr; }

            /**
             * @brief Writes a whole voxelspace (Voxel model) to the stream
             * 
             * @param _Space Space which should be stored.
             * 
             * @return Returns true on success otherwise false.
             */
            virtual bool WriteVoxelSpace(const CVoxelSpace &_Space) { return false; }

            /**
             * @brief Reads a whole voxelspace (Voxel model) from the stream
             * 
             * @param _Space Space which should be filled.
             * 
             * @return Returns true on success otherwise false.
             */
            virtual bool ReadVoxelSpace(CVoxelSpace &_Space) { return false; }

            virtual ~IStreamable() = default;

        protected:
            bool m_IsReadOnly;
    };
} // namespace VCore
