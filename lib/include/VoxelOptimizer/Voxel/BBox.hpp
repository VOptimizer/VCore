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

#ifndef BBOX_HPP
#define BBOX_HPP

#include <VoxelOptimizer/Math/Vector.hpp>

namespace VoxelOptimizer
{
    class CBBox
    {
        public:
            CVectori Beg;
            CVectori End;

            CBBox() = default;
            CBBox(const CBBox &_Other)
            {
                *this = _Other;
            } 
            CBBox(CVectori beg, CVectori end) : Beg(beg), End(end + CVectori(1, 1, 1)) {}

            inline CVector GetSize() const
            {
                return (End - Beg).Max(CVector(1, 1, 1));
            }

            inline bool ContainsPoint(const CVectori &v) const
            {
                return (Beg.x <= v.x && Beg.y <= v.y && Beg.z <= v.z) && (End.x > v.x && End.y > v.y && End.z > v.z);
            }

            inline CBBox& operator=(const CBBox &_Other)
            {
                Beg = _Other.Beg;
                End = _Other.End;
                return *this;
            }

            ~CBBox() = default;
    };
} // namespace VoxelOptimizer


#endif //BBOX_HPP