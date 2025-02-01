/*
 * MIT License
 *
 * Copyright (c) 2021 Christian Tost
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

#ifndef GOXELFORMAT_HPP
#define GOXELFORMAT_HPP

#include <VCore/Formats/IVoxelFormat.hpp>
#include <VCore/Misc/fast_vector.hpp>
#include <string.h>

namespace VCore
{
    class CGoxelFormat : public IVoxelFormat
    {
        public:
            CGoxelFormat() = default;        
            ~CGoxelFormat() = default;

        private:
            struct SChunkHeader
            {
                char Type[4];
                int Size;   // Datasize
            };

            struct BL16
            {
                public:
                    BL16() : m_Data{} {}
                    BL16(uint32_t *_Data) : BL16() { SetData(_Data); }
                    BL16(const BL16 &_Other) { *this = _Other; }
                    BL16(BL16 &&) = default;
                    BL16 &operator=(BL16 &&) = default;

                    BL16 &operator=(const BL16 &_Other)
                    {
                        memcpy(&m_Data[0], _Other.m_Data, CHUNK_SIZE * sizeof(uint32_t));
                        return *this;
                    }

                    inline void SetData(uint32_t *_Data)
                    {
                        memcpy(&m_Data[0], _Data, CHUNK_SIZE * sizeof(uint32_t));
                    }

                    inline uint32_t GetVoxel(Math::Vec3i v)
                    {
                        return m_Data[(size_t)v.x + 16 * (size_t)v.y + 16 * 16 * (size_t)v.z];
                    }

                private:
                    static constexpr uint16_t CHUNK_SIZE = 16 * 16 * 16;

                    uint32_t m_Data[CHUNK_SIZE];
            }; 

            struct Block
            {
                Math::Vec3i Pos;
                int Index;
            };

            struct Layer
            {
                std::vector<Block> Blocks;
                int MatIdx;
                std::string Name;
                bool Visible;
            };

            fast_vector<BL16> m_BL16s;
            fast_vector<Layer> m_Layers;
            ankerl::unordered_dense::map<int, int> m_MeshMaterialMapping;
            CBBox m_BBox;
            bool m_HasEmission;

            void ParseFormat() override;

            void ReadFile();
            void ProcessMaterial(const SChunkHeader &Chunk);
            void ProcessLayer(const SChunkHeader &Chunk);
            void ProcessBL16(const SChunkHeader &Chunk);
            ankerl::unordered_dense::map<std::string, std::string> ReadDict(const SChunkHeader &Chunk, size_t StartPos);
    };
}

#endif //GOXELFORMAT_HPP