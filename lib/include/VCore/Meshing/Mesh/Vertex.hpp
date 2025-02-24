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
#include <string.h>
#include <VCore/Memory/ObjectPool.hpp>

namespace VCore
{
    struct SVertex
    {
        SVertex() = default;
        SVertex(const Math::Vec3f &_Pos, const Math::Vec3f &_Normal, const uint32_t _Color, const uint8_t _AmbientOcclusionValue = 3) : Pos(_Pos), Normal(_Normal), Color(_Color), AmbientOcclusionValue(_AmbientOcclusionValue) {}
        SVertex(SVertex &&) = default;
        SVertex(const SVertex &) = default;

        SVertex &operator=(SVertex &&) = default;
        SVertex &operator=(const SVertex &) = default;

        Math::Vec3f Pos;
        Math::Vec3f Normal;
        uint32_t Color;
        uint8_t AmbientOcclusionValue;

        void *operator new(size_t)
        {            
            return s_Pool.Allocate();
        }

        void operator delete(void *p)
        {
            s_Pool.Deallocate(p);
        }

        inline bool operator==(const SVertex &_Vertex) const
        {
            return _Vertex.Pos == Pos && _Vertex.Normal == Normal && _Vertex.Color == Color && _Vertex.AmbientOcclusionValue == AmbientOcclusionValue;
        }

        private:
            static Memory::CObjectPool<SVertex, 1000> s_Pool;
    };

    inline Memory::CObjectPool<SVertex, 1000> SVertex::s_Pool;

    struct VertexHasher
    {
        size_t operator()(const SVertex &_Vertex) const
        {
            Math::Vec3fHasher v3fhasher;

            size_t ph = v3fhasher(_Vertex.Pos);
            size_t nh = v3fhasher(_Vertex.Normal);

            return ((ph * 73856093) ^ (nh * 19349663) ^ (_Vertex.Color * 83492791) ^ (_Vertex.AmbientOcclusionValue * 5860394));
        }
    };
} // namespace VCore


#endif