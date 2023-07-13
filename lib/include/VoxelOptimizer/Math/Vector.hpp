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

#include <math.h>
#include <unordered_map>

namespace VoxelOptimizer
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
                TVector2(T x, T y) : x(x), y(y) {}

                template<class O>
                TVector2(const TVector2<O> &v) : x(v.x), y(v.y) {}
                TVector2(const TVector2 &_Vec) : x(_Vec.x), y(_Vec.y) {}

                // Upon here just math. Math is magic :D

                inline TVector2 operator*(const TVector2 &vr) const
                {
                    return TVector2(x * vr.x, y * vr.y);
                }

                inline TVector2 operator-(const TVector2 &vr) const
                {
                    return TVector2(x - vr.x, y - vr.y);
                }

                inline TVector2 operator+(const TVector2 &vr) const
                {
                    return TVector2(x + vr.x, y + vr.y);
                }

                inline TVector2 &operator+=(const TVector2 &vr)
                {
                    x += vr.x;
                    y += vr.y;

                    return *this;
                }

                inline bool operator>=(const TVector2 &vr) const
                {
                    if(x != vr.x)
                        return x >= vr.x;

                    return y >= vr.y;
                }

                inline TVector2 operator/(const TVector2 &vr) const
                {
                    return TVector2(x / vr.x, y / vr.y);
                }

                inline TVector2 operator/(float scalar) const
                {
                    return TVector2(x / scalar, y / scalar);
                }

                inline TVector2 operator*(float scalar) const
                {
                    return TVector2(x * scalar, y * scalar);
                }

                inline bool operator==(const TVector2 &vr) const
                {
                    return x == vr.x && y == vr.y;
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

                TVector3() : x(0), y(0), z(0) {}
                TVector3(T x, T y, T z) : x(x), y(y), z(z) {}
                TVector3(const TVector3 &v) : x(v.x), y(v.y), z(v.z) {}

                template<class O>
                TVector3(const TVector3<O> &v) : x(v.x), y(v.y), z(v.z) {}
                
                inline bool IsZero() const
                {
                    return x == 0 && y == 0 && z == 0;
                }

                inline bool operator==(const TVector3 &vr) const
                {
                    return x == vr.x && y == vr.y && z == vr.z;
                }

                inline bool operator!=(const TVector3 &vr) const
                {
                    return x != vr.x || y != vr.y || z != vr.z;
                }

                inline bool operator>(const TVector3 &vr) const
                {
                    if(x != vr.x)
                        return x > vr.x;

                    if(y != vr.y)
                        return y > vr.y;

                    return z > vr.z;
                }

                inline bool operator>=(const TVector3 &vr) const
                {
                    if(x != vr.x)
                        return x >= vr.x;

                    if(y != vr.y)
                        return y >= vr.y;

                    return z >= vr.z;
                }

                inline bool operator<(const TVector3 &vr) const
                {
                    if(x != vr.x)
                        return x < vr.x;

                    if(y != vr.y)
                        return y < vr.y;

                    return z < vr.z;
                }

                inline bool operator<=(const TVector3 &vr) const
                {
                    if(x != vr.x)
                        return x <= vr.x;

                    if(y != vr.y)
                        return y <= vr.y;

                    return z <= vr.z;
                }

                // Upon here just math. Math is magic :D

                inline TVector3 operator*(const TVector3 &vr) const
                {
                    return TVector3(x * vr.x, y * vr.y, z * vr.z);
                }

                inline TVector3 operator-(const TVector3 &vr) const
                {
                    return TVector3(x - vr.x, y - vr.y, z - vr.z);
                }

                inline TVector3 operator+(const TVector3 &vr) const
                {
                    return TVector3(x + vr.x, y + vr.y, z + vr.z);
                }

                inline TVector3 &operator+=(const TVector3 &vr)
                {
                    x += vr.x;
                    y += vr.y;
                    z += vr.z;

                    return *this;
                }

                inline TVector3 &operator*=(const TVector3 &vr)
                {
                    x *= vr.x;
                    y *= vr.y;
                    z *= vr.z;

                    return *this;
                }

                inline TVector3 operator/(const TVector3 &vr) const
                {
                    return TVector3(x / vr.x, y / vr.y, z / vr.z);
                }

                inline TVector3 operator/(float scalar) const
                {
                    return TVector3(x / scalar, y / scalar, z / scalar);
                }

                inline TVector3 operator*(float scalar) const
                {
                    return TVector3(x * scalar, y * scalar, z * scalar);
                }

                inline TVector3 operator-() const
                {
                    return TVector3(-x, -y, -z);
                }

                inline float dot(const TVector3 &vr) const
                {
                    return x * vr.x + y * vr.y + z * vr.z;
                }

                inline TVector3 cross(const TVector3 &vr) const
                {
                    return TVector3(y * vr.z - z * vr.y, 
                                z * vr.x - x * vr.z, 
                                x * vr.y - y * vr.x);
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

                inline TVector3 min(const TVector3 &vec) const
                {
                    return TVector3(std::min(x, vec.x), std::min(y, vec.y), std::min(z, vec.z));
                }

                inline TVector3 max(const TVector3 &vec) const
                {
                    return TVector3(std::max(x, vec.x), std::max(y, vec.y), std::max(z, vec.z));
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

                Vec4f() : x(0), y(0), z(0), w(0) {}
                Vec4f(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

                template<class T>
                Vec4f(const TVector3<T> &v) : x(v.x), y(v.y), z(v.z), w(1.0) {}
                Vec4f(const Vec4f &v) : x(v.x), y(v.y), z(v.z), w(v.w) {}

                template<class T>
                inline TVector3<T> ToVector3() const
                {
                    return TVector3<T> (x, y, z);
                }

                // Upon here just math. Math is magic :D

                inline Vec4f operator*(const Vec4f &vr) const
                {
                    return Vec4f(x * vr.x, y * vr.y, z * vr.z, w * vr.w);
                }

                inline Vec4f operator-(const Vec4f &vr) const
                {
                    return Vec4f(x - vr.x, y - vr.y, z - vr.z, w - vr.w);
                }

                inline Vec4f operator+(const Vec4f &vr) const
                {
                    return Vec4f(x + vr.x, y + vr.y, z + vr.z, w + vr.w);
                }

                inline Vec4f &operator+=(const Vec4f &vr)
                {
                    x += vr.x;
                    y += vr.y;
                    z += vr.z;
                    w += vr.w;

                    return *this;
                }

                inline Vec4f operator/(const Vec4f &vr) const
                {
                    return Vec4f(x / vr.x, y / vr.y, z / vr.z, w / vr.w);
                }

                inline Vec4f operator/(float scalar) const
                {
                    return Vec4f(x / scalar, y / scalar, z / scalar, w / scalar);
                }

                inline Vec4f operator*(float scalar) const
                {
                    return Vec4f(x * scalar, y * scalar, z * scalar, w * scalar);
                }

                ~Vec4f() = default;
        };

        using Vec2f = TVector2<float>;
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
        inline TVector3<T> operator*(float scalar, const TVector3<T> &vr)
        {
            return vr * scalar;
        }

        inline Vec3f floor(const Vec3f &_Vec)
        {
            return Vec3f(floorf(_Vec.x), floorf(_Vec.y), floorf(_Vec.z));
        }

        inline Vec3f fract(const Vec3f &_Vec)
        {
            return Vec3f(_Vec.x - (int)_Vec.x, _Vec.y - (int)_Vec.y, _Vec.z - (int)_Vec.z);
        }

        class Vec2fHasher
        {
            public:
                std::size_t operator()(Vec2f const& _Vec) const noexcept
                {
                    union Floatconvert
                    {
                        float f;
                        int i;
                    };

                    Floatconvert x, y, z;
                    x.f = _Vec.x;
                    y.f = _Vec.y;

                    return (x.i * 73856093) ^ (y.i * 19349663);
                }
        };

        class Vec3fHasher
        {
            public:
                std::size_t operator()(Vec3f const& _Vec) const noexcept
                {
                    union Floatconvert
                    {
                        float f;
                        int i;
                    };

                    Floatconvert x, y, z;
                    x.f = _Vec.x;
                    y.f = _Vec.y;
                    z.f = _Vec.z;
                    

                    // http://www.beosil.com/download/CollisionDetectionHashing_VMV03.pdf
                    return ((x.i * 73856093) ^ (y.i * 19349663) ^ (z.i * 83492791));
                }
        };

        class Vec3iHasher
        {
            public:
                std::size_t operator()(Vec3i const& _Vec) const noexcept
                {
                    // http://www.beosil.com/download/CollisionDetectionHashing_VMV03.pdf
                    return ((_Vec.x * 73856093) ^ (_Vec.y * 19349663) ^ (_Vec.z * 83492791));
                }
        };
    }

    template<class T, class KeyEqual = std::equal_to<Math::Vec3f>>
    using VectorMap = std::unordered_map<Math::Vec3f, T, Math::Vec3fHasher, KeyEqual>;

    template<class T, class KeyEqual = std::equal_to<Math::Vec3i>>
    using VectoriMap = std::unordered_map<Math::Vec3i, T, Math::Vec3iHasher, KeyEqual>;
}

#endif //VECTOR_HPP