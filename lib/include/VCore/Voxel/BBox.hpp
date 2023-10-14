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

#include <VCore/Math/Vector.hpp>

namespace VCore
{
    class CBBox
    {
        public:
            Math::Vec3i Beg;
            Math::Vec3i End;

            CBBox() = default;
            CBBox(const CBBox &_Other)
            {
                *this = _Other;
            } 
            CBBox(Math::Vec3i _Beg, Math::Vec3i _End) : Beg(_Beg), End(_End) {}

            /**
             * @return Gets the size of this bounding box.
             */
            inline Math::Vec3f GetSize() const
            {
                // A voxel has at least the size of one.
                return (End - Beg).max(Math::Vec3f(1, 1, 1));
            }

            /**
             * @brief Returns true if the bounding box contains the given point.
             */
            inline bool ContainsPoint(const Math::Vec3i &_Vec) const
            {
                return (_Vec.x >= Beg.x && _Vec.y >= Beg.y && _Vec.z >= Beg.z) && (_Vec.x < End.x && _Vec.y < End.y && _Vec.z < End.z);
            }

            inline Math::Vec3f GetExtents() const
            {
                return Math::Vec3f(End) - GetCenter();
            }

            inline Math::Vec3f GetCenter() const 
            {
                return ((Math::Vec3f(Beg) + Math::Vec3f(End)) * 0.5f);
            }

            inline CBBox& operator=(const CBBox &_Other)
            {
                Beg = _Other.Beg;
                End = _Other.End;
                return *this;
            }

            ~CBBox() = default;
    };
}


#endif //BBOX_HPP