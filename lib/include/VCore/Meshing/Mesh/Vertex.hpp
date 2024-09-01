/*
 * MIT License
 *
 * Copyright (c) 2024 Christian Tost
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

#ifndef VERTEX_HPP
#define VERTEX_HPP

#include <VCore/Math/Vector.hpp>

namespace VCore
{
    struct SVertex
    {
        SVertex() = default;
        SVertex(const Math::Vec3f &_Pos, const Math::Vec3f &_Normal, const Math::Vec2f &_UV, const Math::Vec2f &_UV2 = Math::Vec2f()) : Pos(_Pos), Normal(_Normal), UV(_UV), UV2(_UV2) {}

        Math::Vec3f Pos;
        Math::Vec3f Normal;
        Math::Vec2f UV, UV2;

        inline bool operator==(const SVertex &_Vertex) const
        {
            return _Vertex.Pos == Pos && _Vertex.Normal == Normal && _Vertex.UV == UV;
        }
    };

    struct VertexHasher
    {
        size_t operator()(const SVertex &_Vertex) const
        {
            Math::Vec3fHasher v3fhasher;
            Math::Vec2fHasher v2fhasher;

            size_t ph = v3fhasher(_Vertex.Pos);
            size_t nh = v3fhasher(_Vertex.Normal);
            size_t uvh = v2fhasher(_Vertex.UV);

            return ((ph * 73856093) ^ (nh * 19349663) ^ (uvh * 83492791));
        }
    };
} // namespace VCore


#endif