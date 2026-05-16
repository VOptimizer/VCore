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

#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <cmath>
#include <cmath>
#include <VCore/Misc/unordered_dense.h>

// #include "SIMD/x86/Vector-SIMD.hpp"
#include <cstdint>
#include <cstring>

namespace VCore
{
    namespace Math
    {
        template<class T>
        class TVector2
        {
            public:
                union
                {
                    struct
                    {
                        T x;
                        T y;
                    };
                    
                    T v[2];
                };

                TVector2() : x(0), y(0){}
                TVector2(T p_x, T p_y) : x(p_x), y(p_y) {}

                template<class O>
                TVector2(const TVector2<O> &p_v) : x(p_v.x), y(p_v.y) {}
                TVector2(const TVector2 &p_Vec) : x(p_Vec.x), y(p_Vec.y) {}

                // Upon here just math. Math is magic :D

                inline float length() const
                {
                    return sqrt((x * x) + (y * y));
                }

                inline TVector2 operator*(const TVector2 &p_vr) const
                {
                    return TVector2(x * p_vr.x, y * p_vr.y);
                }

                inline TVector2 operator-(const TVector2 &p_vr) const
                {
                    return TVector2(x - p_vr.x, y - p_vr.y);
                }

                inline TVector2 operator+(const TVector2 &p_vr) const
                {
                    return TVector2(x + p_vr.x, y + p_vr.y);
                }

                inline TVector2 &operator+=(const TVector2 &p_vr)
                {
                    x += p_vr.x;
                    y += p_vr.y;

                    return *this;
                }

                inline TVector2 &operator=(const TVector2 &p_vr)
                {
                    x = p_vr.x;
                    y = p_vr.y;

                    return *this;
                }

                inline bool operator>=(const TVector2 &p_vr) const
                {
                    if(x != p_vr.x)
                        return x >= p_vr.x;

                    return y >= p_vr.y;
                }

                inline bool operator>(const TVector2 &p_vr) const
                {
                    if(x != p_vr.x)
                        return x > p_vr.x;

                    return y > p_vr.y;
                }

                inline bool operator<(const TVector2 &p_vr) const
                {
                    if(x != p_vr.x)
                        return x < p_vr.x;

                    return y < p_vr.y;
                }

                inline TVector2 operator/(const TVector2 &p_vr) const
                {
                    return TVector2(x / p_vr.x, y / p_vr.y);
                }

                inline TVector2 operator/(float p_scalar) const
                {
                    return TVector2(x / p_scalar, y / p_scalar);
                }

                inline TVector2 operator*(float p_scalar) const
                {
                    return TVector2(x * p_scalar, y * p_scalar);
                }

                inline T &operator[](int idx)
                {
                    return v[idx];
                }

                inline const T &operator[](int idx) const
                {
                    return v[idx];
                }

                inline bool operator==(const TVector2 &p_vr) const
                {
                    return x == p_vr.x && y == p_vr.y;
                }

                ~TVector2() = default;
        };

        template<class T>
        class TVector3
        {
            public:
                static const TVector3 ZERO;
                static const TVector3 ONE;
                static const TVector3 UP;
                static const TVector3 DOWN;
                static const TVector3 FRONT;
                static const TVector3 BACK;
                static const TVector3 LEFT;
                static const TVector3 RIGHT;

                template <typename H>
                friend H AbslHashValue(H h, const TVector3& c) {
                    return H::combine(std::move(h), c.x, c.y, c.z);
                }

                union
                {
                    struct
                    {
                        T x;
                        T y;
                        T z;
                    };
                    
                    T v[3];
                };

                constexpr TVector3() : x(0), y(0), z(0) {}
                constexpr TVector3(T p_x, T p_y, T p_z) : x(p_x), y(p_y), z(p_z) {}
                TVector3(const TVector3 &p_v) : x(p_v.x), y(p_v.y), z(p_v.z) {}

                template<class O>
                TVector3(const TVector3<O> &p_v) : x(p_v.x), y(p_v.y), z(p_v.z) {}
                
                inline bool IsZero() const
                {
                    return x == 0 && y == 0 && z == 0;
                }

                inline bool operator==(const TVector3 &p_vr) const
                {
                    // T va[4] = {}, vb[4] = {};
                    // memmove(va, v, sizeof(T) * 3);
                    // memmove(vb, vr.v, sizeof(T) * 3);
                    // return VectorEq(v, vr.v);

                    return x == p_vr.x && y == p_vr.y && z == p_vr.z;
                }

                inline bool operator!=(const TVector3 &p_vr) const
                {
                    return x != p_vr.x || y != p_vr.y || z != p_vr.z;
                }

                inline bool operator>(const TVector3 &p_vr) const
                {
                    if(x != p_vr.x)
                        return x > p_vr.x;

                    if(y != p_vr.y)
                        return y > p_vr.y;

                    return z > p_vr.z;
                }

                inline bool operator>=(const TVector3 &p_vr) const
                {
                    if(x != p_vr.x)
                        return x >= p_vr.x;

                    if(y != p_vr.y)
                        return y >= p_vr.y;

                    return z >= p_vr.z;
                }

                inline bool operator<(const TVector3 &p_vr) const
                {
                    if(x != p_vr.x)
                        return x < p_vr.x;

                    if(y != p_vr.y)
                        return y < p_vr.y;

                    return z < p_vr.z;
                }

                inline bool operator<=(const TVector3 &p_vr) const
                {
                    if(x != p_vr.x)
                        return x <= p_vr.x;

                    if(y != p_vr.y)
                        return y <= p_vr.y;

                    return z <= p_vr.z;
                }

                // Upon here just math. Math is magic :D

                inline TVector3 operator*(const TVector3 &p_vr) const
                {
                    return TVector3(x * p_vr.x, y * p_vr.y, z * p_vr.z);
                }

                inline TVector3 operator-(const TVector3 &p_vr) const
                {
                    return TVector3(x - p_vr.x, y - p_vr.y, z - p_vr.z);
                    // T out[4];
                    // VectorSub(v, vr.v, out);
                    // return TVector3(out[0], out[1], out[2]);
                }

                inline TVector3 operator+(const TVector3 &p_vr) const
                {
                    return TVector3(x + p_vr.x, y + p_vr.y, z + p_vr.z);
                    // T out[4];
                    // VectorAdd(v, vr.v, out);
                    // return TVector3(out[0], out[1], out[2]);
                }

                inline TVector3 &operator+=(const TVector3 &p_vr)
                {
                    x += p_vr.x;
                    y += p_vr.y;
                    z += p_vr.z;

                    return *this;
                }

                inline TVector3 &operator*=(const TVector3 &p_vr)
                {
                    x *= p_vr.x;
                    y *= p_vr.y;
                    z *= p_vr.z;

                    return *this;
                }

                inline TVector3 &operator=(const TVector3 &p_vr)
                {
                    x = p_vr.x;
                    y = p_vr.y;
                    z = p_vr.z;

                    return *this;
                }

                inline TVector3 operator/(const TVector3 &p_vr) const
                {
                    return TVector3(x / p_vr.x, y / p_vr.y, z / p_vr.z);
                }

                inline TVector3 operator/(float p_scalar) const
                {
                    return TVector3(x / p_scalar, y / p_scalar, z / p_scalar);
                }

                inline TVector3 operator*(float p_scalar) const
                {
                    return TVector3(x * p_scalar, y * p_scalar, z * p_scalar);
                }

                inline TVector3 operator&(T p_scalar) const
                {
                    return TVector3(x & p_scalar, y & p_scalar, z & p_scalar);
                }

                inline TVector3 operator-() const
                {
                    return TVector3(-x, -y, -z);
                }

                inline T &operator[](int idx)
                {
                    return v[idx];
                }

                inline const T &operator[](int idx) const
                {
                    return v[idx];
                }

                inline float dot(const TVector3 &p_vr) const
                {
                    return x * p_vr.x + y * p_vr.y + z * p_vr.z;
                }

                inline TVector3 cross(const TVector3 &p_vr) const
                {
                    return TVector3(y * p_vr.z - z * p_vr.y, 
                                z * p_vr.x - x * p_vr.z, 
                                x * p_vr.y - y * p_vr.x);
                }

                inline float length() const
                {
                    return sqrt((x * x) + (y * y) + (z * z));
                }

                inline TVector3 normalize() const
                {
                    float v = length();
                    if(v == 0)
                        return *this;

                    return TVector3(x / v, y / v, z / v);
                }

                inline TVector3 min(const TVector3 &p_vec) const
                {
                    return TVector3(std::min(x, p_vec.x), std::min(y, p_vec.y), std::min(z, p_vec.z));
                }

                inline TVector3 max(const TVector3 &p_vec) const
                {
                    return TVector3(std::max(x, p_vec.x), std::max(y, p_vec.y), std::max(z, p_vec.z));
                }

                inline TVector3 abs() const
                {
                    return TVector3(fabs(x), fabs(y), fabs(z));
                }

                inline TVector3 &zero_approx()
                {
                    if(fabs(x) < 0.00001f)
                        x = 0;

                    if(fabs(y) < 0.00001f)
                        y = 0;

                    if(fabs(z) < 0.00001f)
                        z = 0;

                    return *this;
                }

                inline TVector2<T> octahedron_encode() const
                {
                    auto nAbs = fabs(x) + fabs(y) + fabs(z);
                    auto p = TVector2<T>(x / nAbs, y / nAbs);
                    if(z < 0)
                        p = TVector2<T>((1.f - fabs(p.y)) * ((p.x < 0) ? -1 : 1), (1.f - fabs(p.x)) * ((p.y < 0) ? -1 : 1));

                    p.x *= 0.5f + 0.5f;
                    p.y *= 0.5f + 0.5f;
                    return p;
                }

                ~TVector3() = default;
        };

        class Vec4f
        {
            public:
                union
                {
                    struct
                    {
                        float x;
                        float y;
                        float z;
                        float w;
                    };
                    
                    float v[4];
                };

                constexpr Vec4f() : x(0), y(0), z(0), w(0) {}
                constexpr Vec4f(float p_x, float p_y, float p_z, float p_w) : x(p_x), y(p_y), z(p_z), w(p_w) {}

                template<class T>
                Vec4f(const TVector3<T> &p_v) : x(p_v.x), y(p_v.y), z(p_v.z), w(1.0) {}
                Vec4f(const Vec4f &p_v) : x(p_v.x), y(p_v.y), z(p_v.z), w(p_v.w) {}

                template<class T>
                inline TVector3<T> ToVector3() const
                {
                    return TVector3<T> (x, y, z);
                }

                // Upon here just math. Math is magic :D

                inline Vec4f operator*(const Vec4f &p_vr) const
                {
                    return Vec4f(x * p_vr.x, y * p_vr.y, z * p_vr.z, w * p_vr.w);
                }

                inline Vec4f operator-(const Vec4f &p_vr) const
                {
                    return Vec4f(x - p_vr.x, y - p_vr.y, z - p_vr.z, w - p_vr.w);
                }

                inline Vec4f operator+(const Vec4f &p_vr) const
                {
                    return Vec4f(x + p_vr.x, y + p_vr.y, z + p_vr.z, w + p_vr.w);
                }

                inline Vec4f &operator+=(const Vec4f &p_vr)
                {
                    x += p_vr.x;
                    y += p_vr.y;
                    z += p_vr.z;
                    w += p_vr.w;

                    return *this;
                }

                inline Vec4f &operator=(const Vec4f &p_vr)
                {
                    x = p_vr.x;
                    y = p_vr.y;
                    z = p_vr.z;
                    w = p_vr.w;

                    return *this;
                }

                inline Vec4f operator/(const Vec4f &p_vr) const
                {
                    return Vec4f(x / p_vr.x, y / p_vr.y, z / p_vr.z, w / p_vr.w);
                }

                inline Vec4f operator/(float p_scalar) const
                {
                    return Vec4f(x / p_scalar, y / p_scalar, z / p_scalar, w / p_scalar);
                }

                inline Vec4f operator*(float p_scalar) const
                {
                    return Vec4f(x * p_scalar, y * p_scalar, z * p_scalar, w * p_scalar);
                }

                inline float &operator[](int idx)
                {
                    return v[idx];
                }

                inline const float &operator[](int idx) const
                {
                    return v[idx];
                }

                ~Vec4f() = default;
        };

        using Vec2f = TVector2<float>;
        using Vec2i = TVector2<int>;
        using Vec2ui = TVector2<unsigned int>;

        using Vec3f = TVector3<float>;
        using Vec3i = TVector3<int>;

        template<class T>   
        const TVector3<T> TVector3<T>::ZERO;

        template<class T>   
        const TVector3<T> TVector3<T>::ONE(1, 1, 1);

        template<class T>   
        const TVector3<T> TVector3<T>::UP(0, 1, 0);

        template<class T>   
        const TVector3<T> TVector3<T>::DOWN(0, -1, 0);

        template<class T>   
        const TVector3<T> TVector3<T>::FRONT(0, 0, 1);

        template<class T>   
        const TVector3<T> TVector3<T>::BACK(0, 0, -1);

        template<class T>   
        const TVector3<T> TVector3<T>::LEFT(-1, 0, 0);

        template<class T>   
        const TVector3<T> TVector3<T>::RIGHT(1, 0, 0);

        template<class T>
        inline TVector3<T> operator*(float p_scalar, const TVector3<T> &p_vr)
        {
            return p_vr * p_scalar;
        }

        inline Vec3f floor(const Vec3f &p_Vec)
        {
            return Vec3f(floorf(p_Vec.x), floorf(p_Vec.y), floorf(p_Vec.z));
        }

        inline Vec3f round(const Vec3f &p_Vec)
        {
            return Vec3f(roundf(p_Vec.x), roundf(p_Vec.y), roundf(p_Vec.z));
        }

        inline Vec3f fract(const Vec3f &p_Vec)
        {
            return Vec3f(p_Vec.x - (int)p_Vec.x, p_Vec.y - (int)p_Vec.y, p_Vec.z - (int)p_Vec.z);
        }

        class Vec2fHasher
        {
            public:
                uint64_t operator()(Vec2f const& p_Vec) const noexcept
                {
                    union Floatconvert
                    {
                        float f;
                        int i;
                    };

                    Floatconvert x, y;
                    x.f = p_Vec.x;
                    y.f = p_Vec.y;

                    return (x.i * 73856093) ^ (y.i * 19349663);
                }
        };

        class Vec3fHasher
        {
            public:
                uint64_t operator()(Vec3f const& p_Vec) const noexcept
                {
                    union Floatconvert
                    {
                        float f;
                        int i;
                    };

                    Floatconvert x, y, z;
                    x.f = p_Vec.x;
                    y.f = p_Vec.y;
                    z.f = p_Vec.z;
                    

                    // http://www.beosil.com/download/CollisionDetectionHashing_VMV03.pdf
                    return ((x.i * 73856093) ^ (y.i * 19349663) ^ (z.i * 83492791));
                }
        };

        class Vec3iHasher
        {
            public:
                uint64_t operator()(Vec3i const& p_Vec) const noexcept
                {
                    // http://www.beosil.com/download/CollisionDetectionHashing_VMV03.pdf
                    return ((p_Vec.x * 73856093) ^ (p_Vec.y * 19349663) ^ (p_Vec.z * 83492791));
                }
        };
    }

    template<class T, class KeyEqual = std::equal_to<Math::Vec3f>>
    using VectorMap = ankerl::unordered_dense::map<Math::Vec3f, T, Math::Vec3fHasher, KeyEqual>;

    template<class T, class KeyEqual = std::equal_to<Math::Vec3i>>
    using VectoriMap = ankerl::unordered_dense::map<Math::Vec3i, T, Math::Vec3iHasher, KeyEqual>;
}

#endif //VECTOR_HPP