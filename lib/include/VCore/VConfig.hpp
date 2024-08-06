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

#include <VCore/Misc/fast_vector.hpp>

// The following macros allow you to use your engine or framework's mesh data structures instead of the V-Core's.

#ifndef CHUNK_SIZE
#define CHUNK_SIZE 30
#endif

#define FACE_MASK ((1 << CHUNK_SIZE) - 1)

/**
 * @brief Defines how data of a vertex should be stored.
 */
#ifndef VERTEX_DATA
#define VERTEX_DATA fast_vector<SVertex> Vertices;
#endif

/**
 * @brief Defines how to move vertex data.
 */
#ifndef VERTEX_MOVE_VERTEX_DATADATA
#define MOVE_VERTEX_DATA Vertices = std::move(_Other.Vertices);
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
        Vertices.push_back(_Vertex);        \
    }
#endif

/**
 * @brief Defines how to access a vertex by its index.
 */
#ifndef GET_VERTEX_DATA_METHOD
#define GET_VERTEX_DATA_METHOD              \
    SVertex operator[](size_t _idx) const  \
    {                                       \
        return Vertices[_idx];              \
    }
#endif

/**
 * @brief Allows certain exporters to get a stream of all the vertices.
 */
#ifndef GET_VERTEX_STREAM_METHOD
#define GET_VERTEX_STREAM_METHOD                        \
    const fast_vector<SVertex> &GetVertices() const     \
    {                                                   \
        return Vertices;                                      \
    }
#endif
#endif