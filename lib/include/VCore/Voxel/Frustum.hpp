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
            CPlane(CPlane &&p_Other) { *this = std::move(p_Other); }
            CPlane(const CPlane &p_Other) { *this = p_Other; }
            CPlane(float p_Distance, const Math::Vec3f &p_Normal) : Distance(p_Distance), Normal(p_Normal) {}
            CPlane(const Math::Vec3f &p_Point, const Math::Vec3f &p_Normal) : Normal(p_Normal.normalize())
            {
                Normal.zero_approx();
                Distance = Normal.dot(p_Point);
            }

            float Distance;
            Math::Vec3f Normal;

            inline float SignedDistanceToPlane(const Math::Vec3f &p_Point) const
            {
                return Normal.dot(p_Point) - Distance;
            }

            inline CPlane &operator=(CPlane &&p_Other)
            {
                Distance = p_Other.Distance;
                Normal = p_Other.Normal;

                p_Other.Distance = 0;
                p_Other.Normal = Math::Vec3f();
                return *this;
            }

            inline CPlane &operator=(const CPlane &p_Other)
            {
                Distance = p_Other.Distance;
                Normal = p_Other.Normal;
                return *this;
            }

            ~CPlane() = default;
    };

    class CFrustum
    {
        public:
            CFrustum() = default;
            CFrustum(CFrustum &&p_Other) { *this = std::move(p_Other); }
            CFrustum(const CFrustum &p_Other) { *this = p_Other; }
            CFrustum(const CPlane &p_Near, const CPlane &p_Far, const CPlane &p_Left, const CPlane &p_Right, const CPlane &p_Top, const CPlane &p_Bottom) : Near(p_Near), Far(p_Far), Left(p_Left), Right(p_Right), Top(p_Top), Bottom(p_Bottom) {}

            CPlane Near;
            CPlane Far;
            CPlane Left;
            CPlane Right;
            CPlane Top;
            CPlane Bottom;

            inline bool IsOnFrustum(const CBBox &p_BBox) const
            {
                Math::Vec3f center = p_BBox.GetCenter();
                Math::Vec3f extents = p_BBox.GetExtents();

                bool b = IsOnOrForwardPlane(Near, center, extents);
                bool b1 = IsOnOrForwardPlane(Left, center, extents);
                bool b2 = IsOnOrForwardPlane(Top, center, extents);
                bool b3 = IsOnOrForwardPlane(Far, center, extents);
                bool b4 = IsOnOrForwardPlane(Right, center, extents);
                bool b5 = IsOnOrForwardPlane(Bottom, center, extents);

                return b && b1 && b2 && b3 && b4 && b5;
            }

            inline static CFrustum Create(const Math::Vec3f &p_CamPosition, const Math::Vec3f &p_CamFront, const Math::Vec3f &p_CamRight, const Math::Vec3f &p_CamUp, float p_Aspect, float p_Fov, float p_Near, float p_Far)
            {
                CFrustum frustum;
                const float halfVSide = p_Far * tanf(p_Fov * 0.5f);
                const float halfHSide = halfVSide * p_Aspect;
                const Math::Vec3f frontMultiplierFar = p_Far * p_CamFront;

                frustum.Near = CPlane(p_CamPosition + p_Near * p_CamFront, p_CamFront);
                frustum.Far = CPlane(p_CamPosition + frontMultiplierFar, -p_CamFront);

                frustum.Right = CPlane(p_CamPosition, (frontMultiplierFar - p_CamRight * halfHSide).cross(p_CamUp));
                frustum.Left = CPlane(p_CamPosition, p_CamUp.cross(frontMultiplierFar + p_CamRight * halfHSide));

                frustum.Top = CPlane(p_CamPosition, p_CamRight.cross(frontMultiplierFar - p_CamUp * halfVSide));
                frustum.Bottom = CPlane(p_CamPosition, (frontMultiplierFar + p_CamUp * halfVSide).cross(p_CamRight));

                return frustum;
            }

            inline CFrustum &operator=(CFrustum &&p_Other)
            {
                Near = std::move(p_Other.Near);
                Far = std::move(p_Other.Far);

                Left = std::move(p_Other.Left);
                Right = std::move(p_Other.Right);

                Top = std::move(p_Other.Top);
                Bottom = std::move(p_Other.Bottom);
                return *this;
            }

            inline CFrustum &operator=(const CFrustum &p_Other)
            {
                Near = p_Other.Near;
                Far = p_Other.Far;

                Left = p_Other.Left;
                Right = p_Other.Right;

                Top = p_Other.Top;
                Bottom = p_Other.Bottom;
                return *this;
            }

            ~CFrustum() = default;

        private:
            inline bool IsOnOrForwardPlane(const CPlane &p_Plane, const Math::Vec3f &p_Center, const Math::Vec3f &p_Extend) const
            {
                // https://gdbooks.gitbooks.io/3dcollisions/content/Chapter2/static_aabb_plane.html
                // https://learnopengl.com/Guest-Articles/2021/Scene/Frustum-Culling
                Math::Vec3f extendNormal = p_Extend * p_Plane.Normal.abs();
                float r = extendNormal.x + extendNormal.y + extendNormal.z;

                return -r <= p_Plane.SignedDistanceToPlane(p_Center);
            }
    };
} // namespace VCore


#endif