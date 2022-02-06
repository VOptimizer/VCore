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

#ifndef OPTIMIZEMESHER_HPP
#define OPTIMIZEMESHER_HPP

#include <list>
#include <VoxelOptimizer/Meshers/IMesher.hpp>
#include <thread>

namespace VoxelOptimizer
{
    class CGreedyMesher : public IMesher
    {
        public:
            CGreedyMesher() = default;

            std::map<CVector, Mesh> GenerateMeshes(VoxelMesh m) override;

            ~CGreedyMesher() = default;

        private:
            struct SSlice
            {
                public:
                    SSlice() : BBox(CVector(INFINITY, INFINITY, INFINITY), CVector()) {}

                    CBBox BBox;
                    std::list<CBBox> Transparent;
            };

            std::vector<Mesh> MeshThread(VoxelMesh m, int axis, const std::map<float, SSlice> &slices); 

            std::mutex m_Lock;

            std::map<float, SSlice> m_XSlices;
            std::map<float, SSlice> m_YSlices;
            std::map<float, SSlice> m_ZSlices;

            std::map<CVector, Voxel> m_Voxels;

            void GenerateMesh(Mesh RetMesh, VoxelMesh m, const CBBox &BBox, bool Opaque);

            Mesh GenerateSliceMesh(const SSlice &slice, VoxelMesh m, int axis);

            void GenerateSlices();
    };
} // namespace VoxelOptimizer

#endif //OPTIMIZEMESHER_HPP