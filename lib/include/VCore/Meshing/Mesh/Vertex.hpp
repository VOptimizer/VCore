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
#include <cstring>
#include <VCore/Memory/ObjectPool.hpp>

namespace VCore
{
    struct SVertex
    {
        SVertex() = default;
        SVertex(const Math::Vec3f &p_Pos, const Math::Vec3f &p_Normal, const uint32_t p_Color, const uint8_t p_AmbientOcclusionValue = 3) : Pos(p_Pos), Normal(p_Normal), Color(p_Color), AmbientOcclusionValue(p_AmbientOcclusionValue) {}
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

        void operator delete(void *p_Ptr)
        {
            s_Pool.Deallocate(p_Ptr);
        }

        inline bool operator==(const SVertex &p_Vertex) const
        {
            return p_Vertex.Pos == Pos && p_Vertex.Normal == Normal && p_Vertex.Color == Color && p_Vertex.AmbientOcclusionValue == AmbientOcclusionValue;
        }

        private:
            static Memory::CObjectPool<SVertex, 1000> s_Pool;
    };

    inline Memory::CObjectPool<SVertex, 1000> SVertex::s_Pool;

    struct VertexHasher
    {
        size_t operator()(const SVertex &p_Vertex) const
        {
            Math::Vec3fHasher v3fhasher;

            size_t ph = v3fhasher(p_Vertex.Pos);
            size_t nh = v3fhasher(p_Vertex.Normal);

            return ((ph * 73856093) ^ (nh * 19349663) ^ (p_Vertex.Color * 83492791) ^ (p_Vertex.AmbientOcclusionValue * 5860394));
        }
    };
} // namespace VCore


#endif