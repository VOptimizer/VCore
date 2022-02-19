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

#ifndef MAT4X4_HPP
#define MAT4X4_HPP

#include <VoxelOptimizer/Math/Vector.hpp>

namespace VoxelOptimizer
{
    class CMat4x4
    {
        public:
            CVector4 x;  // First row
            CVector4 y;  // Second row
            CVector4 z;  // Third row
            CVector4 w;  // Fourth row

            CMat4x4() : x(1, 0, 0, 0), y(0, 1, 0, 0), z(0, 0, 1, 0), w(0, 0, 0, 1) {}
            CMat4x4(const CVector4 &x, const CVector4 &y, const CVector4 &z, const CVector4 &w) : x(x), y(y), z(z), w(w) {}
            CMat4x4(const CMat4x4 &mat) : x(mat.x), y(mat.y), z(mat.z), w(mat.w) {}

            inline CMat4x4 &operator*=(const CMat4x4 &mat)
            {
                // auto c1 = MultiplyVector(mat.x);
                // auto c2 = MultiplyVector(mat.y);
                // auto c3 = MultiplyVector(mat.z);
                // auto c4 = MultiplyVector(mat.w);

                // x = CVector4(c1.x, c2.x, c3.x, c4.x);
                // y = CVector4(c1.y, c2.y, c3.y, c4.y);
                // z = CVector4(c1.z, c2.z, c3.z, c4.z);
                // w = CVector4(c1.w, c2.w, c3.w, c4.w);

                CVector4 v1(mat.x.x, mat.y.x, mat.z.x, mat.w.x);
                CVector4 v2(mat.x.y, mat.y.y, mat.z.y, mat.w.y);
                CVector4 v3(mat.x.z, mat.y.z, mat.z.z, mat.w.z);
                CVector4 v4(mat.x.w, mat.y.w, mat.z.w, mat.w.w);

                auto c1 = MultiplyVector(v1);
                auto c2 = MultiplyVector(v2);
                auto c3 = MultiplyVector(v3);
                auto c4 = MultiplyVector(v4);

                x = CVector4(c1.x, c2.x, c3.x, c4.x);
                y = CVector4(c1.y, c2.y, c3.y, c4.y);
                z = CVector4(c1.z, c2.z, c3.z, c4.z);
                w = CVector4(c1.w, c2.w, c3.w, c4.w);

                return *this;
            }

            inline CMat4x4 operator*(const CMat4x4 &mat)
            {
                // auto c1 = MultiplyVector(mat.x);
                // auto c2 = MultiplyVector(mat.y);
                // auto c3 = MultiplyVector(mat.z);
                // auto c4 = MultiplyVector(mat.w);

                // return CMat4x4(CVector4(c1.x, c2.x, c3.x, c4.x),
                //                CVector4(c1.y, c2.y, c3.y, c4.y),
                //                CVector4(c1.z, c2.z, c3.z, c4.z),
                //                CVector4(c1.w, c2.w, c3.w, c4.w));

                CVector4 v1(mat.x.x, mat.y.x, mat.z.x, mat.w.x);
                CVector4 v2(mat.x.y, mat.y.y, mat.z.y, mat.w.y);
                CVector4 v3(mat.x.z, mat.y.z, mat.z.z, mat.w.z);
                CVector4 v4(mat.x.w, mat.y.w, mat.z.w, mat.w.w);

                auto c1 = MultiplyVector(v1);
                auto c2 = MultiplyVector(v2);
                auto c3 = MultiplyVector(v3);
                auto c4 = MultiplyVector(v4);

                return CMat4x4(CVector4(c1.x, c2.x, c3.x, c4.x),
                               CVector4(c1.y, c2.y, c3.y, c4.y),
                               CVector4(c1.z, c2.z, c3.z, c4.z),
                               CVector4(c1.w, c2.w, c3.w, c4.w));

                // return CMat4x4(MultiplyVector(v1), MultiplyVector(v2), MultiplyVector(v3), MultiplyVector(v4));
            }

            inline CMat4x4 &operator+=(const CMat4x4 &mat)
            {
                x += mat.x;
                y += mat.y;
                z += mat.z;
                w += mat.w;

                return *this;
            }

            inline CMat4x4 operator+(const CMat4x4 &mat)
            {
                return CMat4x4(x + mat.x,
                               y + mat.y,
                               z + mat.z,
                               w + mat.w);
            }

            inline CVector operator*(const CVector &vec)
            {
                return MultiplyVector(vec).ToVector3<float>();
            }

            inline static CMat4x4 Translation(const CVector &pos)
            {
                CMat4x4 ret;
                ret.x.w = pos.x;
                ret.y.w = pos.y;
                ret.z.w = pos.z;

                return ret;
            }

            inline static CMat4x4 Scale(const CVector &scale)
            {
                CMat4x4 ret;
                ret.x.x = scale.x;
                ret.y.y = scale.y;
                ret.z.z = scale.z;

                return ret;
            }

            inline CMat4x4 &Rotate(const CVector &axis, float phi)
            {
                float c = cos(phi), s = sin(phi);
                CVector powAxis = axis * axis;

                CMat4x4 rotMat(
                    CVector4(c + powAxis.x * (1 - c), axis.x * axis.y * (1 - c) - axis.z * s, axis.x * axis.z * (1 - c) + axis.y * s, 0),
                    CVector4(axis.y * axis.x * (1 - c) + axis.z * s, c + powAxis.y * (1 - c), axis.y * axis.z * (1 - c) - axis.x * s, 0),
                    CVector4(axis.z * axis.x * (1 - c) - axis.y * s, axis.z * axis.y * (1 - c) + axis.x * s, c + powAxis.z * (1 - c), 0),
                    CVector4(0, 0, 0, 1)
                );

                *this = rotMat * (*this);
                return *this;
            }

            // inline static CMat4x4 Rotation(const CVector &rotation)
            // {
            //     float c, s;

            //     c = cos(rotation.x);
            //     s = sin(rotation.x);

            //     CMat4x4 xmat(
            //         CVector4(1, 0, 0, 0),
            //         CVector4(0, c, -s, 0),
            //         CVector4(0, s, c, 0),
            //         CVector4(0, 0, 0, 1)
            //     );

            //     c = cos(rotation.y);
            //     s = sin(rotation.y);

            //     CMat4x4 ymat(
            //         CVector4(c, 0, s, 0),
            //         CVector4(0, 1, 0, 0),
            //         CVector4(-s, 0, c, 0),
            //         CVector4(0, 0, 0, 1)
            //     );

            //     c = cos(rotation.z);
            //     s = sin(rotation.z);
            //     CMat4x4 zmat(
            //         CVector4(c, -s, 0, 0),
            //         CVector4(s, c, 0, 0),
            //         CVector4(0, 0, 1, 0),
            //         CVector4(0, 0, 0, 1)
            //     );

            //     // return zmat * xmat * ymat; //xmat * (ymat * zmat);

            //     float cx, cy, cz, sx, sy, sz;
                
            //     cx = cos(rotation.x);
            //     sx = sin(rotation.x);

            //     cy = cos(rotation.y);
            //     sy = sin(rotation.y);

            //     cz = cos(rotation.z);
            //     sz = sin(rotation.z);

            //     return CMat4x4(
            //         CVector4(cy * cz, -)
            //     );

            //     // return CMat4x4(CVector4(cos(rotation.y), -sin(rotation.y), 0, 0),
            //     //         CVector4(sin(rotation.y), cos(rotation.y), 0, 0),
            //     //         CVector4(0, 0, 1, 0),
            //     //         CVector4(0, 0, 0, 1)) *
            //     // CMat4x4(CVector4(cos(rotation.z), 0, sin(rotation.z), 0),
            //     //         CVector4(0, 1, 0, 0),
            //     //         CVector4(-sin(rotation.z), 0, cos(rotation.z), 0),
            //     //         CVector4(0, 0, 0, 1)) *
            //     // CMat4x4(CVector4(1, 0, 0, 0),
            //     //         CVector4(0, cos(rotation.x), -sin(rotation.x), 0),
            //     //         CVector4(0, sin(rotation.x), cos(rotation.x), 0),
            //     //         CVector4(0, 0, 0, 1));
            // }

            inline CVector GetEuler()
            {
                CVector rotation;

                // Calculates the euler angle of the roation matrix.
                // Source: http://eecs.qmul.ac.uk/~gslabaugh/publications/euler.pdf (09.10.2021)
                if(z.x != 1 && z.x != -1)
                {
                    rotation.y = -asin(z.x);
                    rotation.x = atan2(z.y / cos(rotation.y), z.z / cos(rotation.y));
                    rotation.z = atan2(y.x / cos(rotation.y), x.x / cos(rotation.y));
                }
                else
                {
                    if(z.x == -1)
                    {
                        rotation.y = M_PI / 2.f;
                        rotation.x = atan2(x.y, x.z);
                    }
                    else
                    {
                        rotation.y = -M_PI / 2.f;
                        rotation.x = atan2(-x.y, -x.z);
                    }
                }

                return rotation;
            }

            // inline CMat4x4 Rotation()
            // {
            //     return Rotation(GetEuler());
            // }

            ~CMat4x4() {}
        private:
            inline CVector4 MultiplyVector(const CVector4 &vec)
            {
                auto v1 = x * vec;
                auto v2 = y * vec;
                auto v3 = z * vec;
                auto v4 = w * vec;

                return CVector4(v1.x + v1.y + v1.z + v1.w, 
                               v2.x + v2.y + v2.z + v2.w, 
                               v3.x + v3.y + v3.z + v3.w,
                               v4.x + v4.y + v4.z + v4.w);
            }
    };
} // namespace VoxelOptimizer


#endif //MAT4X4_HPP