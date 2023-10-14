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

#ifndef QUBICLEFORMAT_HPP
#define QUBICLE_HPP

#include <VCore/Math/Mat4x4.hpp>
#include <VCore/Formats/IVoxelFormat.hpp>

namespace VCore
{
    class CQubicleFormat : public IVoxelFormat
    {
        public:
            CQubicleFormat() = default;
            ~CQubicleFormat() = default;
        private:
            std::map<int, int> m_ColorIdx;
            void ParseFormat() override;

            void LoadNode();
            void LoadModel();
            void LoadMatrix();
            void LoadCompound();

            int GetColorIdx(int color);
            void AddVoxel(VoxelMesh mesh, int color, Math::Vec3i pos, Math::Vec3i &Beg, Math::Vec3i &End);

            Math::Vec3i ReadVector();
    };
}


#endif //QUBICLEFORMAT_HPP