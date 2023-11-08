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

/**
 * @brief Defines how data of a vertex should be stored.
 */
#define VERTEX_DATA                     \
    std::vector<Math::Vec3f> Vertices;  \
    std::vector<Math::Vec3f> Normals;   \
    std::vector<Math::Vec2f> UV;        \

/**
 * @brief Defines how to move vertex data.
 */
#define MOVE_VERTEX_DATA                    \
    Vertices = std::move(_Other.Vertices);  \
    Normals = std::move(_Other.Normals);    \
    UV = std::move(_Other.UV);              \

/**
 * @brief Defines how to get the total size of the vertex buffer.
 */
#define GET_VERTEX_DATA_SIZE_METHOD \
    int Size() const            \
    {                           \
        return Vertices.size(); \
    }

/**
 * @brief Defines how a vertex is added to the vertex list.
 */
#define ADD_VERTEX_DATA_METHOD              \
    void AddVertex(const SVertex &_Vertex)  \
    {                                       \
        Vertices.push_back(_Vertex.Pos);    \
        Normals.push_back(_Vertex.Normal);  \
        UV.push_back(_Vertex.UV);           \
    }

/**
 * @brief Defines how to access a vertex by its index.
 */
#define GET_VERTEX_DATA_METHOD                                      \
    SVertex operator[](size_t _idx) const                           \
    {                                                               \
        return SVertex(Vertices[_idx], Normals[_idx], UV[_idx]);    \
    }

/**
 * @brief Allows certain exporters to get a stream of all the vertices.
 */
#define GET_VERTEX_STREAM_METHOD                                    \
    std::vector<SVertex> GetVertices() const                        \
    {                                                               \
        std::vector<SVertex> ret;                                   \
        for (int i = 0; i < Size(); i++)                            \
            ret.push_back(SVertex(Vertices[i], Normals[i], UV[i])); \
        return ret;                                                 \
    }
#else
#error "ALREADY DEFINED"
#endif