/*
 * MIT License
 *
 * Copyright (c) 2023 Christian Tost
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

#ifndef FLOODMESHER_HPP
#define FLOODMESHER_HPP

#include <queue>
#include <VoxelOptimizer/Meshing/IMesher.hpp>
#include <VoxelOptimizer/Memory/ObjectPool.hpp>

namespace VoxelOptimizer
{
    struct SFloodShape
    {
        CVector Normal;
        std::map<CVector, Voxel> m_Voxels; 
    };

    class CFloodMesher : public IMesher
    {
        public:
            CFloodMesher() = default;

            std::map<CVector, Mesh> GenerateMeshes(VoxelMesh m, bool onlyDirty = false) override;

            virtual ~CFloodMesher() = default;

        private:
            CObjectPool<SFloodShape> m_ShapesPool;

            std::queue<CVectori> m_Colors;
            std::list<SFloodShape*> m_Shapes;
            std::map<CVector, bool> m_Visited;

            void GroupShapes(const CVectori &_Position);
            std::vector<std::vector<CVector>> GeneratePolygons(SFloodShape *_Shape);
    };
}

#endif