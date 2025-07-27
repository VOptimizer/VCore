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

#ifndef SIMPLEMESHER_HPP
#define SIMPLEMESHER_HPP

#include <VCore/Meshing/IMesher.hpp>
#include <VCore/Meshing/Mesh/MeshBuilder.hpp>

namespace VCore
{
    class CSimpleMesher : public IMesher
    {
        public:
            CSimpleMesher() : IMesher() {}
            virtual ~CSimpleMesher() = default;

        protected:
            struct IndexPair
            {
                IndexPair() : Idx2(0), Idx4(0), AO2(0), AO4(0), Instantiated(false) {}
                IndexPair(uint32_t p_Idx2, uint32_t p_Idx4, uint8_t p_AO2, uint8_t p_AO4) : Idx2(p_Idx2), Idx4(p_Idx4), AO2(p_AO2), AO4(p_AO4), Instantiated(true) {}
                IndexPair(IndexPair &&) = default;
                IndexPair(const IndexPair &) = default;

                IndexPair& operator=(IndexPair &&) = default;
                IndexPair& operator=(const IndexPair &) = default;

                uint32_t Idx2;
                uint32_t Idx4;
                uint8_t AO2;
                uint8_t AO4;
                bool Instantiated;
            };

            void GenerateQuads(CMeshBuilder &p_Builder, Config::bitmask_t p_Faces, int p_Depth, int p_Width, bool p_isFront, const Math::Vec3i &p_Axis, const SChunkMeta &p_Chunk, const VoxelModel &p_Model, const CVoxel& p_Voxel, IndexPair *p_Cache);

            SMeshChunk GenerateMeshChunk(VoxelModel p_Mesh, const SChunkMeta &p_Chunk, bool) override;
    };
}


#endif //SIMPLEMESHER_HPP