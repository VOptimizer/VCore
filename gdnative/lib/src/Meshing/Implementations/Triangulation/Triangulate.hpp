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

#ifndef TRIANGULATE_HPP
#define TRIANGULATE_HPP

#include <vector>
#include <VoxelOptimizer/Math/Vector.hpp>

/**
 * https://www.flipcode.com/archives/Efficient_Polygon_Triangulation.shtml
 */

namespace VoxelOptimizer
{
    class CTriangulate
    {
        public:
            CTriangulate() = delete;
            
            /**
             * @brief Triangulates a polygon.
             */
            static bool Triangulate(const std::vector<CVector> &_polygon, std::vector<int> &_indices);

            /**
             * @brief Computes the area of a polygon
             */
            static float Area(const std::vector<CVector> &_polygon);

            /**
             * @brief Checks if a given point is inside a triangle.
             */
            static bool PointInTriangle(const CVector &_p, const CVector &_a, const CVector &_b, const CVector &_c);

            static float Cross2D(const CVector &_a, const CVector &_b);
        private:
            static bool Snip(const std::vector<CVector> &contour,int u,int v,int w,int n,int *V);
    };
} // namespace VoxelOptimizer


#endif