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

#include <functional>
#include <string>
#include <math.h>
#include <iostream>

namespace VoxelOptimizer
{
    template<class T>
    class TVector
    {
        public:
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

            TVector() : x(0), y(0), z(0) {}
            TVector(T x, T y, T z) : x(x), y(y), z(z) {}
            TVector(const TVector &v) : x(v.x), y(v.y), z(v.z) {}

            template<class O>
            TVector(const TVector<O> &v) : x(v.x), y(v.y), z(v.z) {}
            
            inline TVector Floor()
            {
                return TVector(floorf(x), floorf(y), floorf(z));
            }

            inline TVector Sign()
            {
                return TVector(Sign(x), Sign(y), Sign(z));
            }

            inline TVector Fract()
            {
                return TVector(x - (int)x, y - (int)y, z - (int)z);
            }

            inline bool IsZero() const
            {
                return x == 0 && y == 0 && z == 0;
            }

            inline bool operator==(const TVector &vr) const
            {
                return x == vr.x && y == vr.y && z == vr.z;
            }

            inline bool operator!=(const TVector &vr) const
            {
                return x != vr.x || y != vr.y || z != vr.z;
            }

            inline bool operator>(const TVector &vr) const
            {
                if(x != vr.x)
                    return x > vr.x;

                if(y != vr.y)
                    return y > vr.y;

                return z > vr.z;

                return (x > vr.x) || (y > vr.y) || (z > vr.z);
            }

            inline bool operator>=(const TVector &vr) const
            {
                if(x != vr.x)
                    return x >= vr.x;

                if(y != vr.y)
                    return y >= vr.y;

                return z >= vr.z;

                return (x >= vr.x) || (y >= vr.y) || (z >= vr.z);
            }

            inline bool operator<(const TVector &vr) const
            {
                if(x != vr.x)
                    return x < vr.x;

                if(y != vr.y)
                    return y < vr.y;

                return z < vr.z;

                return (x < vr.x) || (y < vr.y) || (z < vr.z);
            }

            inline bool operator<=(const TVector &vr) const
            {
                if(x != vr.x)
                    return x <= vr.x;

                if(y != vr.y)
                    return y <= vr.y;

                return z <= vr.z;

                return (x <= vr.x) || (y <= vr.y) || (z <= vr.z);
            }

            // Upon here just math. Math is magic :D

            inline TVector operator*(const TVector &vr) const
            {
                return TVector(x * vr.x, y * vr.y, z * vr.z);
            }

            inline TVector operator-(const TVector &vr) const
            {
                return TVector(x - vr.x, y - vr.y, z - vr.z);
            }

            inline TVector operator+(const TVector &vr) const
            {
                return TVector(x + vr.x, y + vr.y, z + vr.z);
            }

            inline TVector &operator+=(const TVector &vr)
            {
                x += vr.x;
                y += vr.y;
                z += vr.z;

                return *this;
            }

            inline TVector operator/(const TVector &vr) const
            {
                return TVector(x / vr.x, y / vr.y, z / vr.z);
            }

            inline TVector operator/(float scalar) const
            {
                return TVector(x / scalar, y / scalar, z / scalar);
            }

            inline TVector operator*(float scalar) const
            {
                return TVector(x * scalar, y * scalar, z * scalar);
            }

            inline TVector operator-() const
            {
                return TVector(-x, -y, -z);
            }

            inline float Dot(const TVector &vr) const
            {
                return x * vr.x + y * vr.y + z * vr.z;
            }

            inline TVector Cross(const TVector &vr) const
            {
                return TVector(y * vr.z - z * vr.y, 
                               z * vr.x - x * vr.z, 
                               x * vr.y - y * vr.x);
            }

            inline float Length() const
            {
                return sqrt((x * x) + (y * y) + (z * z));
            }

            inline TVector Normalize() const
            {
                float v = Length();
                return TVector(x / v, y / v, z / v);
            }

            inline size_t hash() const
            {
                std::hash<std::string> Hash;
                return Hash(std::to_string(x) + "_" + std::to_string(y) + "_" + std::to_string(z));
            }

            inline TVector Min(const TVector &vec) const
            {
                return TVector(std::min(x, vec.x), std::min(y, vec.y), std::min(z, vec.z));
            }

            inline TVector Max(const TVector &vec) const
            {
                return TVector(std::max(x, vec.x), std::max(y, vec.y), std::max(z, vec.z));
            }

            inline TVector Abs() const
            {
                return TVector(fabs(x), fabs(y), fabs(z));
            }

            ~TVector() = default;

        private:
            inline float Sign(float v)
            {
                return v == 0 ? 0 : v < 0 ? -1 : 1;
            }
    };

    template<class T>
    inline TVector<T> operator*(float scalar, const TVector<T> &vr)
    {
        return vr * scalar;
    }

    using CVector = TVector<float>;
    using CVectori = TVector<int>;

    class CVector4
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

            CVector4() : x(0), y(0), z(0), w(0) {}
            CVector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

            template<class T>
            CVector4(const TVector<T> &v) : x(v.x), y(v.y), z(v.z), w(1.0) {}
            CVector4(const CVector4 &v) : x(v.x), y(v.y), z(v.z), w(v.w) {}

            template<class T>
            inline TVector<T> ToVector3() const
            {
                return TVector<T> (x, y, z);
            }

            // Upon here just math. Math is magic :D

            inline CVector4 operator*(const CVector4 &vr) const
            {
                return CVector4(x * vr.x, y * vr.y, z * vr.z, w * vr.w);
            }

            inline CVector4 operator-(const CVector4 &vr) const
            {
                return CVector4(x - vr.x, y - vr.y, z - vr.z, w - vr.w);
            }

            inline CVector4 operator+(const CVector4 &vr) const
            {
                return CVector4(x + vr.x, y + vr.y, z + vr.z, w + vr.w);
            }

            inline CVector4 &operator+=(const CVector4 &vr)
            {
                x += vr.x;
                y += vr.y;
                z += vr.z;
                w += vr.w;

                return *this;
            }

            inline CVector4 operator/(const CVector4 &vr) const
            {
                return CVector4(x / vr.x, y / vr.y, z / vr.z, w / vr.w);
            }

            inline CVector4 operator/(float scalar) const
            {
                return CVector4(x / scalar, y / scalar, z / scalar, w / scalar);
            }

            inline CVector4 operator*(float scalar) const
            {
                return CVector4(x * scalar, y * scalar, z * scalar, w * scalar);
            }

            ~CVector4() = default;
    };

    template<class T>
    inline std::ostream &operator<<(std::ostream &of, const TVector<T> &vec)
    {
        of << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
        return of;
    }
} // namespace VoxelOptimizer

#endif //VECTOR_HPP