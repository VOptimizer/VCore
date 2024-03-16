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

#ifndef HALFEDGE_HPP
#define HALFEDGE_HPP

#include <VCore/Math/Vector.hpp>
#include <VCore/Meshing/Mesh.hpp>

#include <unordered_map>

namespace VCore
{
    // class CHalfEdge;
    // class CHalfFace;

    class CHalfVertex
    {
        public:
            CHalfVertex(const Math::Vec3f &_Pos) : Pos(_Pos) {}

            Math::Vec3f Pos;
            std::unordered_map<uint64_t, std::pair<unsigned int, bool>> Indices;
            // CHalfEdge *Edge;
    };

    // class CHalfEdge
    // {
    //     public:
    //         CHalfEdge(CHalfVertex *_Origin) : Origin(_Origin), Pair(nullptr), Face(nullptr), Next(nullptr), Prev(nullptr) {}

    //         void AddNeighbors(CHalfEdge *_Next, CHalfEdge *_Prev)
    //         {
    //             Next = _Next;
    //             Prev = _Prev;
    //         }

    //         CHalfVertex *Origin;
    //         CHalfEdge *Pair;
    //         CHalfFace *Face;

    //         CHalfEdge *Next, *Prev;
    // };

    class CHalfFace
    {
        public:
            CHalfFace(const Math::Vec3f &_Normal, const Math::Vec2f &_UV) : Normal(_Normal), UV(_UV), Color(0), Vertices(), m_Hash(0) { CalculateHash(); }

            uint64_t GetHash() const
            {
                return m_Hash;
            }

            Math::Vec3f Normal;
            Math::Vec2f UV;
            unsigned int Color;

            CHalfVertex* Vertices[3];

        private:
            void CalculateHash()
            {
                Math::Vec3fHasher v3fhasher;
                Math::Vec2fHasher v2fhasher;

                size_t nh = v3fhasher(Normal);
                size_t uvh = v2fhasher(UV);

                m_Hash = ((nh * 19349663L) ^ (uvh * 83492791L));
            }

            uint64_t m_Hash;
    };

    class CHalfMesh
    {
        public:
            CHalfMesh() : m_IndexCounter(0) {}

            /**
             * @brief Adds a new face to the mesh.
             * The vertices should be in order so _v1 -> _v2 -> _v3 -> _v1. The winding order should be clockwise.
             */
            void AddFace(const SVertex &_v1, const SVertex &_v2, const SVertex &_v3);

            Mesh Build();

            ~CHalfMesh() { Clear(); }
        
        private:
            CHalfVertex* FindOrAddVertex(const Math::Vec3f &_Pos);
            void CheckHash(CHalfVertex* _v, uint64_t _Hash);

            void AddVertex(CHalfVertex* _v);
            void Clear();

            unsigned int m_IndexCounter;

            std::vector<CHalfFace*> m_Faces;
            // std::vector<CHalfEdge*> m_Edges;
            std::unordered_map<int, std::vector<CHalfVertex*>> m_Vertices;
    };
} // namespace VCore

#endif