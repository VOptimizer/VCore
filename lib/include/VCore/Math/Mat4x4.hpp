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
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
                Mat4x4(const Mat4x4 &p_Mat) : x(p_Mat.x), y(p_Mat.y), z(p_Mat.z), w(p_Mat.w) {}

                inline Mat4x4 &operator*=(const Mat4x4 &p_Mat)
                {
                    Vec4f v1(p_Mat.x.x, p_Mat.y.x, p_Mat.z.x, p_Mat.w.x);
                    Vec4f v2(p_Mat.x.y, p_Mat.y.y, p_Mat.z.y, p_Mat.w.y);
                    Vec4f v3(p_Mat.x.z, p_Mat.y.z, p_Mat.z.z, p_Mat.w.z);
                    Vec4f v4(p_Mat.x.w, p_Mat.y.w, p_Mat.z.w, p_Mat.w.w);

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

                inline Mat4x4 operator*(const Mat4x4 &p_Mat)
                {
                    Vec4f v1(p_Mat.x.x, p_Mat.y.x, p_Mat.z.x, p_Mat.w.x);
                    Vec4f v2(p_Mat.x.y, p_Mat.y.y, p_Mat.z.y, p_Mat.w.y);
                    Vec4f v3(p_Mat.x.z, p_Mat.y.z, p_Mat.z.z, p_Mat.w.z);
                    Vec4f v4(p_Mat.x.w, p_Mat.y.w, p_Mat.z.w, p_Mat.w.w);

                    auto c1 = MultiplyVector(v1);
                    auto c2 = MultiplyVector(v2);
                    auto c3 = MultiplyVector(v3);
                    auto c4 = MultiplyVector(v4);

                    return Mat4x4(Vec4f(c1.x, c2.x, c3.x, c4.x),
                                Vec4f(c1.y, c2.y, c3.y, c4.y),
                                Vec4f(c1.z, c2.z, c3.z, c4.z),
                                Vec4f(c1.w, c2.w, c3.w, c4.w));
                }

                inline Mat4x4 operator*(float p_Scalar)
                {
                    return Mat4x4(x * p_Scalar, y * p_Scalar, z * p_Scalar, w * p_Scalar);
                }

                inline Mat4x4 &operator+=(const Mat4x4 &p_Mat)
                {
                    x += p_Mat.x;
                    y += p_Mat.y;
                    z += p_Mat.z;
                    w += p_Mat.w;

                    return *this;
                }

                inline Mat4x4 &operator=(const Mat4x4 &p_Mat)
                {
                    x = p_Mat.x;
                    y = p_Mat.y;
                    z = p_Mat.z;
                    w = p_Mat.w;

                    return *this;
                }


                inline Mat4x4 operator+(const Mat4x4 &p_Mat)
                {
                    return Mat4x4(x + p_Mat.x,
                                y + p_Mat.y,
                                z + p_Mat.z,
                                w + p_Mat.w);
                }

                inline Mat4x4 operator-(const Mat4x4 &p_Mat)
                {
                    return Mat4x4(x - p_Mat.x,
                                y - p_Mat.y,
                                z - p_Mat.z,
                                w - p_Mat.w);
                }

                inline Vec3f operator*(const Vec3f &p_Vec)
                {
                    return MultiplyVector(p_Vec).ToVector3<float>();
                }

                inline static Mat4x4 Translation(const Vec3f &p_Pos)
                {
                    Mat4x4 ret;
                    ret.x.w = p_Pos.x;
                    ret.y.w = p_Pos.y;
                    ret.z.w = p_Pos.z;

                    return ret;
                }

                inline static Mat4x4 Scale(const Vec3f &p_Scale)
                {
                    Mat4x4 ret;
                    ret.x.x *= p_Scale.x;
                    ret.y.y *= p_Scale.y;
                    ret.z.z *= p_Scale.z;

                    return ret;
                }

                inline Mat4x4 &Rotate(const Vec3f &p_Axis, float p_Phi)
                {
                    float c = cos(p_Phi), s = sin(p_Phi);
                    Vec3f powAxis = p_Axis * p_Axis;

                    Mat4x4 rotMat(
                        Vec4f(c + powAxis.x * (1 - c), p_Axis.x * p_Axis.y * (1 - c) - p_Axis.z * s, p_Axis.x * p_Axis.z * (1 - c) + p_Axis.y * s, 0),
                        Vec4f(p_Axis.y * p_Axis.x * (1 - c) + p_Axis.z * s, c + powAxis.y * (1 - c), p_Axis.y * p_Axis.z * (1 - c) - p_Axis.x * s, 0),
                        Vec4f(p_Axis.z * p_Axis.x * (1 - c) - p_Axis.y * s, p_Axis.z * p_Axis.y * (1 - c) + p_Axis.x * s, c + powAxis.z * (1 - c), 0),
                        Vec4f(0, 0, 0, 1)
                    );

                    *this = rotMat * (*this);
                    return *this;
                }

                inline Vec3f GetScale() const
                {
                    Vec3f c1(x.x, y.x, z.x);
                    Vec3f c2(x.y, y.y, z.y);
                    Vec3f c3(x.z, y.z, z.z);

                    float xsign = 1.f;
                    if(Det3x3() < 0)
                        xsign = -1.f;

                    return Vec3f(c1.length() * xsign, c2.length(), c3.length());
                }

                inline Vec3f GetEuler() const
                {
                    auto scale = GetScale();
                    Mat4x4 rotationMatrix(
                        x / scale,
                        y / scale,
                        z / scale,
                        Vec4f(0, 0, 0, 1)
                    );
                    Vec3f rotation;

                    // Calculates the euler angle of the roation matrix.
                    // Source: http://eecs.qmul.ac.uk/~gslabaugh/publications/euler.pdf (09.10.2021)
                    if(rotationMatrix.z.x != 1 && rotationMatrix.z.x != -1)
                    {
                        rotation.y = -asin(rotationMatrix.z.x);
                        rotation.x = atan2(rotationMatrix.z.y / cos(rotation.y), rotationMatrix.z.z / cos(rotation.y));
                        rotation.z = atan2(rotationMatrix.y.x / cos(rotation.y), rotationMatrix.x.x / cos(rotation.y));
                    }
                    else
                    {
                        if(z.x == -1)
                        {
                            rotation.y = M_PI / 2.f;
                            rotation.x = atan2(rotationMatrix.x.y, rotationMatrix.x.z);
                        }
                        else
                        {
                            rotation.y = -M_PI / 2.f;
                            rotation.x = atan2(-rotationMatrix.x.y, -rotationMatrix.x.z);
                        }
                    }

                    return rotation;
                }

                ~Mat4x4() {}
            private:
                // inline Mat4x4 Transpose() const
                // {
                //     return Mat4x4(
                //                 Vec4f(x.x, y.x, z.x, w.x),
                //                 Vec4f(x.y, y.y, z.y, w.y),
                //                 Vec4f(x.z, y.z, z.z, w.z),
                //                 Vec4f(x.w, y.w, z.w, w.w)
                //             );
                // }

                inline double Det3x3() const
                {
                    return x.x * (y.y * z.z - y.z * z.y) -
                           x.y * (y.x * z.z - y.z * z.x) +
                           x.z * (y.x * z.y - y.y * z.x);
                }

                // inline Mat4x4 Inverse() const
                // {
                //     float det = Det3x3();
                //     if(det == 0.0)
                //         return Mat4x4();
                //     float invDet = 1.f / det;

                //     Mat4x4 result(
                //         Vec4f(invDet * (y.y * z.z - y.z * z.y), -invDet * (x.y * z.z - x.z * z.y), invDet * (x.y * y.z - x.z * y.y), 0.f),
                //         Vec4f(-invDet * (y.x * z.z - y.z * z.x), invDet * (x.x * z.z - x.z * z.x), -invDet * (x.x * y.z - x.z * y.x), 0.f),
                //         Vec4f(invDet * (y.x * z.y - y.y * z.x), -invDet * (x.x * z.y - x.y * z.x), invDet * (x.x * y.y - x.y * y.x), 0.f),
                //         Vec4f( 0, 0, 0, 1.f)
                //     );

                //     result.w = Vec4f(
                //         -(w.x * result.x.x + w.y * result.y.x + w.z * result.z.x),
                //         -(w.x * result.x.y + w.y * result.y.y + w.z * result.z.y),
                //         -(w.x * result.x.z + w.y * result.y.z + w.z * result.z.z),
                //         1.f
                //     );

                //     return result;
                // }

                inline Vec4f MultiplyVector(const Vec4f &p_Vec)
                {
                    auto v1 = x * p_Vec;
                    auto v2 = y * p_Vec;
                    auto v3 = z * p_Vec;
                    auto v4 = w * p_Vec;

                    return Vec4f(v1.x + v1.y + v1.z + v1.w, 
                                v2.x + v2.y + v2.z + v2.w, 
                                v3.x + v3.y + v3.z + v3.w,
                                v4.x + v4.y + v4.z + v4.w);
                }

                // inline double AbsMax() const
                // {
                //     double m = fabs(x.x);
                //     m = std::max(m, fabs(x.y));
                //     m = std::max(m, fabs(x.z));
                //     m = std::max(m, fabs(x.w));
                //     m = std::max(m, fabs(y.x));
                //     m = std::max(m, fabs(y.y));
                //     m = std::max(m, fabs(y.z));
                //     m = std::max(m, fabs(y.w));
                //     m = std::max(m, fabs(z.x));
                //     m = std::max(m, fabs(z.y));
                //     m = std::max(m, fabs(z.z));
                //     m = std::max(m, fabs(z.w));
                //     m = std::max(m, fabs(w.x));
                //     m = std::max(m, fabs(w.y));
                //     m = std::max(m, fabs(w.z));
                //     m = std::max(m, fabs(w.w));
                //     return m;
                // }

                // inline Mat4x4 Polar() const
                // {
                //     auto r = *this;
                //     for (int i = 0; i < 50; i++) 
                //     {
                //         auto rInverse = r.Transpose().Inverse();
                //         auto rNext = (r + rInverse) * 0.5;
                //         double diff = (rNext - r).AbsMax();
                //         if (diff < 1e-6f)
                //             return rNext;

                //         r = rNext;
                //     }
                //     return r;
                // }
        };
    }
}

#endif //MAT4X4_HPP