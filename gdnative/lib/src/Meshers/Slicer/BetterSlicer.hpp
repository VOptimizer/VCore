/*
 * MIT License
 *
 * Copyright (c) 2022 Christian Tost
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

#ifndef BETTERSLICER_HPP
#define BETTERSLICER_HPP

#include <vector>
#include <utility>

#include <VoxelOptimizer/Loaders/VoxelMesh.hpp>
#include <VoxelOptimizer/Vector.hpp>

namespace VoxelOptimizer
{
    class CBetterSlicer
    {
        public:
            CBetterSlicer(VoxelMesh Mesh, bool Opaque, const std::map<CVectori, Voxel> &voxels) : m_Mesh(Mesh), m_Opaque(Opaque), m_Voxels(voxels) {}

            /**
             * @brief Sets the axis which is currently the main axis.
             */
            void SetActiveAxis(int Axis);

            /**
             * @return Returns true if there is a face on the given position.
             */
            bool IsFace(CVector Pos, bool secondFace = false);

            /**
             * @brief Adds a quad to the list of already processed ones.
             */
            void AddProcessedQuad(CVector Pos, CVector Size);

            void ClearQuads();

            inline CVector Normal()
            {
                return m_Normal;
            }

            inline int Material()
            {
                return m_Material;
            }

            inline int Color()
            {
                return m_Color;
            }
            
            ~CBetterSlicer() {}
        private:
            Voxel GetVoxel(const CVector &v);
            Voxel GetVoxelOrFromMesh(const CVector &v);
            void SetFaceNormal(Voxel v, bool IsCurrent);

            VoxelMesh m_Mesh;
            CVector m_Neighbour;
            int m_Axis;
            CVector m_Normal;
            int m_Material;
            int m_Color;
            bool m_Opaque;

            const std::map<CVectori, Voxel> &m_Voxels;

            std::vector<std::pair<CVector, CVector>> m_ProcessedQuads;
    };
} // namespace VoxelOptimizer

#endif //BETTERSLICER_HPP