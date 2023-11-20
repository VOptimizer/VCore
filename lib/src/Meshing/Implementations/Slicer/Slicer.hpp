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

#ifndef SLICER_HPP
#define SLICER_HPP

#include <vector>
#include <utility>

#include <VCore/Voxel/VoxelModel.hpp>
#include <VCore/Math/Vector.hpp>

namespace VCore
{
    class CSlicer
    {
        public:
            CSlicer(VoxelModel Mesh, bool Opaque, const CChunk *_Chunk, const CBBox &_TotalBBox) : m_Mesh(Mesh), m_Opaque(Opaque), m_Chunk(_Chunk), m_TotalBBox(_TotalBBox), m_Size(_TotalBBox.GetSize())
            {
                // auto &voxels = m_Mesh->GetVoxels();
                // m_Voxels = voxels.queryVisible();
            }

            /**
             * @brief Sets the axis which is currently the main axis.
             */
            void SetActiveAxis(int Axis);

            /**
             * @return Returns true if there is a face on the given position.
             */
            bool IsFace(Math::Vec3i Pos);

            /**
             * @brief Adds a quad to the list of already processed ones.
             */
            void AddProcessedQuad(Math::Vec3i Pos, Math::Vec3i Size);

            void ClearQuads();

            inline Math::Vec3i Normal()
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

            ~CSlicer() = default;
        private:
            void SetFaceNormal(Voxel v, bool IsCurrent);
            bool IsTransparentFaceVisible(Voxel v);

            Voxel GetVoxel(const Math::Vec3i &v);

            VoxelModel m_Mesh;
            Math::Vec3i m_Neighbour;
            int m_Axis;
            Math::Vec3i m_Normal;
            int m_Material;
            int m_Color;
            bool m_Opaque;
            bool m_HasFace;

            std::vector<std::pair<Math::Vec3i, Math::Vec3i>> m_ProcessedQuads;
            const CChunk *m_Chunk;
            const CBBox &m_TotalBBox;
            const Math::Vec3i m_Size;
    };
}


#endif //SLICER_HPP