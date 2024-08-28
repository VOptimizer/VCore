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
#include <VCore/Meshing/MeshBuilder.hpp>

namespace VCore
{
    class CSimpleMesher : public IMesher
    {
        public:
            CSimpleMesher() : IMesher() {}
            virtual ~CSimpleMesher() = default;

        protected:
            void GenerateQuads(CMeshBuilder &_Builder, BITMASK_TYPE _Faces, int depth, int width, bool isFront, const Math::Vec3i &_Axis, const SChunkMeta &_Chunk, const VoxelModel &_Model, const std::vector<std::string> &_Parts);

            SMeshChunk GenerateMeshChunk(VoxelModel m, const SChunkMeta &_Chunk, bool Opaque) override;
    };
}


#endif //SIMPLEMESHER_HPP