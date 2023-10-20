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

#include <VCore/Math/Vector.hpp>
#define M_PI 3.14159265358979323846

namespace VCore
{
    namespace Math
    {
        class Mat4x4
        {
            public:
                Vec4f x;  // First row
                Vec4f y;  // Second row
                Vec4f z;  // Third row
                Vec4f w;  // Fourth row

                Mat4x4() : x(1, 0, 0, 0), y(0, 1, 0, 0), z(0, 0, 1, 0), w(0, 0, 0, 1) {}
                Mat4x4(const Vec4f &x, const Vec4f &y, const Vec4f &z, const Vec4f &w) : x(x), y(y), z(z), w(w) {}
                Mat4x4(const Mat4x4 &mat) : x(mat.x), y(mat.y), z(mat.z), w(mat.w) {}

                inline Mat4x4 &operator*=(const Mat4x4 &mat)
                {
                    Vec4f v1(mat.x.x, mat.y.x, mat.z.x, mat.w.x);
                    Vec4f v2(mat.x.y, mat.y.y, mat.z.y, mat.w.y);
                    Vec4f v3(mat.x.z, mat.y.z, mat.z.z, mat.w.z);
                    Vec4f v4(mat.x.w, mat.y.w, mat.z.w, mat.w.w);

                    auto c1 = MultiplyVector(v1);
                    auto c2 = MultiplyVector(v2);
                    auto c3 = MultiplyVector(v3);
                    auto c4 = MultiplyVector(v4);

                    x = Vec4f(c1.x, c2.x, c3.x, c4.x);
                    y = Vec4f(c1.y, c2.y, c3.y, c4.y);
                    z = Vec4f(c1.z, c2.z, c3.z, c4.z);
                    w = Vec4f(c1.w, c2.w, c3.w, c4.w);

                    return *this;
                }

                inline Mat4x4 operator*(const Mat4x4 &mat)
                {
                    Vec4f v1(mat.x.x, mat.y.x, mat.z.x, mat.w.x);
                    Vec4f v2(mat.x.y, mat.y.y, mat.z.y, mat.w.y);
                    Vec4f v3(mat.x.z, mat.y.z, mat.z.z, mat.w.z);
                    Vec4f v4(mat.x.w, mat.y.w, mat.z.w, mat.w.w);

                    auto c1 = MultiplyVector(v1);
                    auto c2 = MultiplyVector(v2);
                    auto c3 = MultiplyVector(v3);
                    auto c4 = MultiplyVector(v4);

                    return Mat4x4(Vec4f(c1.x, c2.x, c3.x, c4.x),
                                Vec4f(c1.y, c2.y, c3.y, c4.y),
                                Vec4f(c1.z, c2.z, c3.z, c4.z),
                                Vec4f(c1.w, c2.w, c3.w, c4.w));
                }

                inline Mat4x4 &operator+=(const Mat4x4 &mat)
                {
                    x += mat.x;
                    y += mat.y;
                    z += mat.z;
                    w += mat.w;

                    return *this;
                }

                inline Mat4x4 &operator=(const Mat4x4 &mat)
                {
                    x = mat.x;
                    y = mat.y;
                    z = mat.z;
                    w = mat.w;

                    return *this;
                }


                inline Mat4x4 operator+(const Mat4x4 &mat)
                {
                    return Mat4x4(x + mat.x,
                                y + mat.y,
                                z + mat.z,
                                w + mat.w);
                }

                inline Vec3f operator*(const Vec3f &vec)
                {
                    return MultiplyVector(vec).ToVector3<float>();
                }

                inline static Mat4x4 Translation(const Vec3f &pos)
                {
                    Mat4x4 ret;
                    ret.x.w = pos.x;
                    ret.y.w = pos.y;
                    ret.z.w = pos.z;

                    return ret;
                }

                inline static Mat4x4 Scale(const Vec3f &scale)
                {
                    Mat4x4 ret;
                    ret.x.x = scale.x;
                    ret.y.y = scale.y;
                    ret.z.z = scale.z;

                    return ret;
                }

                inline Mat4x4 &Rotate(const Vec3f &axis, float phi)
                {
                    float c = cos(phi), s = sin(phi);
                    Vec3f powAxis = axis * axis;

                    Mat4x4 rotMat(
                        Vec4f(c + powAxis.x * (1 - c), axis.x * axis.y * (1 - c) - axis.z * s, axis.x * axis.z * (1 - c) + axis.y * s, 0),
                        Vec4f(axis.y * axis.x * (1 - c) + axis.z * s, c + powAxis.y * (1 - c), axis.y * axis.z * (1 - c) - axis.x * s, 0),
                        Vec4f(axis.z * axis.x * (1 - c) - axis.y * s, axis.z * axis.y * (1 - c) + axis.x * s, c + powAxis.z * (1 - c), 0),
                        Vec4f(0, 0, 0, 1)
                    );

                    *this = rotMat * (*this);
                    return *this;
                }

                inline Vec3f GetEuler()
                {
                    Vec3f rotation;

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

                ~Mat4x4() {}
            private:
                inline Vec4f MultiplyVector(const Vec4f &vec)
                {
                    auto v1 = x * vec;
                    auto v2 = y * vec;
                    auto v3 = z * vec;
                    auto v4 = w * vec;

                    return Vec4f(v1.x + v1.y + v1.z + v1.w, 
                                v2.x + v2.y + v2.z + v2.w, 
                                v3.x + v3.y + v3.z + v3.w,
                                v4.x + v4.y + v4.z + v4.w);
                }
        };
    }
}

#endif //MAT4X4_HPP