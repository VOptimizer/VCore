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

#ifndef FRUSTUM_HPP
#define FRUSTUM_HPP

#include <VCore/Math/Vector.hpp>
#include <VCore/Voxel/BBox.hpp>

namespace VCore
{
    class CPlane
    {
        public:
            CPlane() : Distance(0) {}
            CPlane(CPlane &&_Other) { *this = std::move(_Other); }
            CPlane(const CPlane &_Other) { *this = _Other; }
            CPlane(float _Distance, const Math::Vec3f &_Normal) : Distance(_Distance), Normal(_Normal) {}
            CPlane(const Math::Vec3f &_Point, const Math::Vec3f &_Normal) : Normal(_Normal.normalize())
            {
                Normal.zero_approx();
                Distance = Normal.dot(_Point);
            }

            float Distance;
            Math::Vec3f Normal;

            inline float SignedDistanceToPlane(const Math::Vec3f &_Point) const
            {
                return Normal.dot(_Point) - Distance;
            }

            inline CPlane &operator=(CPlane &&_Other)
            {
                Distance = _Other.Distance;
                Normal = _Other.Normal;

                _Other.Distance = 0;
                _Other.Normal = Math::Vec3f();
                return *this;
            }

            inline CPlane &operator=(const CPlane &_Other)
            {
                Distance = _Other.Distance;
                Normal = _Other.Normal;
                return *this;
            }

            ~CPlane() = default;
    };

    class CFrustum
    {
        public:
            CFrustum() = default;
            CFrustum(CFrustum &&_Other) { *this = std::move(_Other); }
            CFrustum(const CFrustum &_Other) { *this = _Other; }
            CFrustum(const CPlane &_Near, const CPlane &_Far, const CPlane &_Left, const CPlane &_Right, const CPlane &_Top, const CPlane &_Bottom) : Near(_Near), Far(_Far), Left(_Left), Right(_Right), Top(_Top), Bottom(_Bottom) {}

            CPlane Near;
            CPlane Far;
            CPlane Left;
            CPlane Right;
            CPlane Top;
            CPlane Bottom;

            inline bool IsOnFrustum(const CBBox &_BBox) const
            {
                Math::Vec3f center = _BBox.GetCenter();
                Math::Vec3f extents = _BBox.GetExtents();

                bool b = IsOnOrForwardPlane(Near, center, extents);
                bool b1 = IsOnOrForwardPlane(Left, center, extents);
                bool b2 = IsOnOrForwardPlane(Top, center, extents);
                bool b3 = IsOnOrForwardPlane(Far, center, extents);
                bool b4 = IsOnOrForwardPlane(Right, center, extents);
                bool b5 = IsOnOrForwardPlane(Bottom, center, extents);

                return b && b1 && b2 && b3 && b4 && b5;
            }

            inline static CFrustum Create(const Math::Vec3f &_CamPosition, const Math::Vec3f &_CamFront, const Math::Vec3f &_CamRight, const Math::Vec3f &_CamUp, float _Aspect, float _Fov, float _Near, float _Far)
            {
                CFrustum frustum;
                const float halfVSide = _Far * tanf(_Fov * 0.5f);
                const float halfHSide = halfVSide * _Aspect;
                const Math::Vec3f frontMultiplierFar = _Far * _CamFront;

                frustum.Near = CPlane(_CamPosition + _Near * _CamFront, _CamFront);
                frustum.Far = CPlane(_CamPosition + frontMultiplierFar, -_CamFront);

                frustum.Right = CPlane(_CamPosition, (frontMultiplierFar - _CamRight * halfHSide).cross(_CamUp));
                frustum.Left = CPlane(_CamPosition, _CamUp.cross(frontMultiplierFar + _CamRight * halfHSide));

                frustum.Top = CPlane(_CamPosition, _CamRight.cross(frontMultiplierFar - _CamUp * halfVSide));
                frustum.Bottom = CPlane(_CamPosition, (frontMultiplierFar + _CamUp * halfVSide).cross(_CamRight));

                return frustum;
            }

            inline CFrustum &operator=(CFrustum &&_Other)
            {
                Near = std::move(_Other.Near);
                Far = std::move(_Other.Far);

                Left = std::move(_Other.Left);
                Right = std::move(_Other.Right);

                Top = std::move(_Other.Top);
                Bottom = std::move(_Other.Bottom);
                return *this;
            }

            inline CFrustum &operator=(const CFrustum &_Other)
            {
                Near = _Other.Near;
                Far = _Other.Far;

                Left = _Other.Left;
                Right = _Other.Right;

                Top = _Other.Top;
                Bottom = _Other.Bottom;
                return *this;
            }

            ~CFrustum() = default;

        private:
            inline bool IsOnOrForwardPlane(const CPlane &_Plane, const Math::Vec3f &_Center, const Math::Vec3f &_Extend) const
            {
                // https://gdbooks.gitbooks.io/3dcollisions/content/Chapter2/static_aabb_plane.html
                // https://learnopengl.com/Guest-Articles/2021/Scene/Frustum-Culling
                Math::Vec3f extendNormal = _Extend * _Plane.Normal.abs();
                float r = extendNormal.x + extendNormal.y + extendNormal.z;

                return -r <= _Plane.SignedDistanceToPlane(_Center);
            }
    };
} // namespace VCore


#endif