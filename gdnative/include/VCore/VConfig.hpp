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

#ifndef VCONFIG_HPP
#define VCONFIG_HPP

#include <PoolArrays.hpp>

// The following macros allow you to use your engine or framework's mesh data structures instead of the V-Core's.

/**
 * @brief Defines how data of a vertex should be stored.
 */
#ifndef VERTEX_DATA
#define VERTEX_DATA \
        godot::PoolVector3Array Vertices, Normals; \
        godot::PoolVector2Array UVs;
#endif

/**
 * @brief Defines how to move vertex data.
 */
#ifndef VERTEX_MOVE_VERTEX_DATADATA
#define MOVE_VERTEX_DATA \
        Vertices = _Other.Vertices; \
        Normals = _Other.Normals; \
        UVs = _Other.UVs;
#endif

/**
 * @brief Defines how to get the total size of the vertex buffer.
 */
#ifndef GET_VERTEX_DATA_SIZE_METHOD
#define GET_VERTEX_DATA_SIZE_METHOD \
    int Size() const            \
    {                           \
        return Vertices.size(); \
    }
#endif

/**
 * @brief Defines how a vertex is added to the vertex list.
 */
#ifndef ADD_VERTEX_DATA_METHOD
#define ADD_VERTEX_DATA_METHOD              \
    void AddVertex(const SVertex &_Vertex)  \
    {                                       \
        Vertices.append(godot::Vector3(_Vertex.Pos.x, _Vertex.Pos.y, _Vertex.Pos.z));        \
        Normals.append(godot::Vector3(_Vertex.Normal.x, _Vertex.Normal.y, _Vertex.Normal.z));        \
        UVs.append(godot::Vector2(_Vertex.UV.x, _Vertex.UV.y));        \
    }
#endif

/**
 * @brief Defines how to access a vertex by its index.
 */
#ifndef GET_VERTEX_DATA_METHOD
#define GET_VERTEX_DATA_METHOD              \
    SVertex operator[](size_t _idx) const  \
    {                                       \
        SVertex ret;                        \
        auto gvec = Vertices.read()[_idx];         \
        ret.Pos = Math::Vec3f(gvec.x, gvec.y, gvec.z); \
                                                        \
        gvec = Normals.read()[_idx];         \
        ret.Normal = Math::Vec3f(gvec.x, gvec.y, gvec.z); \
                                                        \
        auto gvec2 = UVs.read()[_idx];         \
        ret.UV = Math::Vec2f(gvec2.x, gvec2.y); \
        return ret;              \
    }
#endif

/**
 * @brief Allows certain exporters to get a stream of all the vertices.
 */
#ifndef GET_VERTEX_STREAM_METHOD
#define GET_VERTEX_STREAM_METHOD                        \
    std::vector<SVertex> GetVertices() const     \
    {                                                   \
        std::vector<SVertex> ret;                        \
        for(int i = 0; i < Size(); i++)                 \
            ret.push_back(this->operator[](i));                     \
        return ret;                                \
    }
#endif
#endif