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

#ifndef SURFACE_HPP
#define SURFACE_HPP

#include "VCore/Math/Vector.hpp"
#include "Vertex.hpp"
#include <VCore/Meshing/Material.hpp>
#include <VCore/VPlatform.hpp>
#include <VCore/Misc/fast_vector.hpp>

#ifdef VCORE_RTTI_ENABLED
#include <stdexcept>
#include <typeindex>
#endif

namespace VCore
{
    class ISurface
    {
        public:        
            uint8_t MaterialHandle; //!< Material of this surface

            /**
             * @brief Adds a new Vertex to the surface.
             * @return Returns the index of the vertex.
             */
            virtual uint32_t AddVertex(const SVertex *p_Vertex) = 0;

            /**
             * @brief Adds a face to the surface. The order must be counter clockwise.
             */
            virtual void AddFace(uint32_t p_Idx1, uint32_t p_Idx2, uint32_t p_Idx3) = 0;

            /**
             * @return Returns the vertex count, which this surface holds. 
             */
            virtual uint64_t GetVertexCount() const = 0;

            /**
             * @return Returns the face count, which this surface holds. 
             */
            virtual uint64_t GetFaceCount() const = 0;

            /**
             * @brief Updates a vertex at a given index.
             */
            virtual void UpdateVertex(uint64_t p_Idx, const SVertex &p_Vertex) = 0;

            /**
             * @return Returns a vertex at the given index.
             */
            virtual SVertex GetVertex(uint64_t p_Idx) const = 0;

            /**
             * @return Returns an index at the given index.
             */
            virtual uint32_t GetIndex(uint64_t p_Idx) const = 0;

            /**
             * @brief Reserve memory for the given _Size of vertices.
             */
            virtual void ReserveVertices(uint64_t p_Size) = 0;

            /**
             * @brief Reserve memory for the given _Size of faces. (_Size * 3) = index count.
             */
            virtual void ReserveFaces(uint64_t p_Size) = 0;

            /**
             * @return Returns the underlying raw pointer of the continous memory stream of the vertices.
             */
            virtual const SVertex* GetRawVertexPointer() const = 0;

            /**
             * @return Returns the underlying raw pointer of the continous memory stream of the indices.
             */
            virtual const void* GetRawIndexPointer() const = 0;

            /**
             * @return Returns true, if the maximum of indices is reached. This happens on webgl and / or opengles based application, since these have sometimes a cap at UINT16_MAX indices.
             */
            virtual bool IsFaceCountMaxReached() const = 0;

            /**
             * @brief Merges two surfaces together.
             */
            virtual void MergeSurface(ISurface *p_Surface) = 0;

            template<class T>
            const T &GetVertexReference() const
            {
#ifdef VCORE_RTTI_ENABLED
                // Safety first.
                if(std::type_index(typeid(T)) != GetUnderlyingVertexType())
                    throw std::runtime_error("Can't cast underlying vertex type!");
#endif
                return *(T*)GetUnderlyingVertexReference();
            }

            template<class T>
            const T &GetIndexReference() const
            {
#ifdef VCORE_RTTI_ENABLED
                // Safety first.
                if(std::type_index(typeid(T)) != GetUnderlyingIndexType())
                    throw std::runtime_error("Can't cast underlying index type!");
#endif
                return *(T*)GetUnderlyingIndexReference();
            }

            virtual void ClaimVertices() = 0;

            virtual ~ISurface() = default;

        protected:
            virtual const void *GetUnderlyingVertexReference() const = 0;
            virtual const void *GetUnderlyingIndexReference() const = 0;

#ifdef VCORE_RTTI_ENABLED
            virtual std::type_index GetUnderlyingVertexType() const = 0;
            virtual std::type_index GetUnderlyingIndexType() const = 0;
#endif
    };

    template<class VertexArray, class IndexArray, uint32_t IndexMax>
    class TSurface : public ISurface
    {
        public:
            virtual ~TSurface()
            {
                for (auto &&v : m_Vertices)
                    delete v;

                m_Vertices.clear();
            }

            uint32_t AddVertex(const SVertex *p_Vertex) override
            {
                m_Vertices.push_back(const_cast<SVertex*>(p_Vertex));
                return m_Vertices.size() - 1;
            }

            void AddFace(uint32_t p_Idx1, uint32_t p_Idx2, uint32_t p_Idx3) override
            {
                m_Indices.push_back(p_Idx1);
                m_Indices.push_back(p_Idx2);
                m_Indices.push_back(p_Idx3);
            }

            uint64_t GetVertexCount() const override
            {
                return (uint64_t)m_Vertices.size();
            }

            uint64_t GetFaceCount() const override
            {
                return (uint64_t)(m_Indices.size() / 3);
            }

            void UpdateVertex(uint64_t p_Idx, const SVertex &p_Vertex) override
            {
                if(p_Idx < m_Vertices.size())
                    *m_Vertices[p_Idx] = p_Vertex;
            }

            SVertex GetVertex(uint64_t p_Idx) const override
            {
                return *m_Vertices[p_Idx];
            }

            uint32_t GetIndex(uint64_t p_Idx) const override
            {
                return m_Indices[p_Idx];
            }

            void ReserveVertices(uint64_t p_Size) override
            {
                m_Vertices.reserve(p_Size);
            }

            void ReserveFaces(uint64_t p_Size) override
            {
                m_Indices.reserve(p_Size * 3);
            }

            const SVertex* GetRawVertexPointer() const override
            {
                return nullptr; //m_Vertices.data();
            }

            const void* GetRawIndexPointer() const override
            {
                return m_Indices.data();
            }

            bool IsFaceCountMaxReached() const override
            {
                return (IndexMax - m_Vertices.size()) > 3;
            }

            void MergeSurface(ISurface *p_Surface) override
            {
                ReserveVertices(p_Surface->GetVertexCount() + GetVertexCount());
                ReserveFaces(p_Surface->GetFaceCount() + GetFaceCount());

                auto &otherVertices = p_Surface->GetVertexReference<VertexArray>();
                auto &otherIndices = p_Surface->GetIndexReference<IndexArray>();

                auto startIdx = m_Vertices.size();
                m_Vertices.insert(m_Vertices.end(), otherVertices.begin(), otherVertices.end());

                for (auto &&i : otherIndices)
                    m_Indices.push_back(startIdx + i);

                p_Surface->ClaimVertices();
            }

            void ClaimVertices() override
            {
                m_Vertices.clear();
                m_Indices.clear();
            }
        protected:
            const void *GetUnderlyingVertexReference() const override
            {
                return &m_Vertices;
            }

            const void *GetUnderlyingIndexReference() const override
            {
                return &m_Indices;
            }

#ifdef VCORE_RTTI_ENABLED
            std::type_index GetUnderlyingVertexType() const override
            {
                return typeid(m_Vertices);
            }

            std::type_index GetUnderlyingIndexType() const override
            {
                return typeid(m_Indices);
            }
#endif
        private:
            VertexArray m_Vertices;
            IndexArray m_Indices;
    };

//     class CArraySurface : public ISurface
//     {
//         public:
//             virtual ~CArraySurface()
//             {
//                 // for (auto &&v : m_Vertices)
//                 //     delete v;

//                 // m_Vertices.clear();
//             }

//             uint32_t AddVertex(const SVertex *p_Vertex) override
//             {
//                 m_Positions.push_back(p_Vertex->Pos);
//                 m_Normals.push_back(p_Vertex->Normal);
//                 m_Colors.push_back(p_Vertex->Color);

//                 delete p_Vertex;

//                 // m_Vertices.push_back(const_cast<SVertex*>(p_Vertex));
//                 return m_Positions.size() - 1;
//             }

//             void AddFace(uint32_t p_Idx1, uint32_t p_Idx2, uint32_t p_Idx3) override
//             {
//                 m_Indices.push_back(p_Idx1);
//                 m_Indices.push_back(p_Idx2);
//                 m_Indices.push_back(p_Idx3);
//             }

//             uint64_t GetVertexCount() const override
//             {
//                 return (uint64_t)m_Positions.size();
//             }

//             uint64_t GetFaceCount() const override
//             {
//                 return (uint64_t)(m_Indices.size() / 3);
//             }

//             void UpdateVertex(uint64_t p_Idx, const SVertex &p_Vertex) override
//             {
//                 if(p_Idx < m_Positions.size())
//                 {
//                     m_Positions[p_Idx] = p_Vertex.Pos;
//                     m_Normals[p_Idx] = p_Vertex.Normal;
//                     m_Colors[p_Idx] = p_Vertex.Color;
//                 }
//             }

//             SVertex GetVertex(uint64_t p_Idx) const override
//             {
//                 return SVertex(m_Positions[p_Idx], m_Normals[p_Idx], m_Colors[p_Idx]);
//             }

//             uint32_t GetIndex(uint64_t p_Idx) const override
//             {
//                 return m_Indices[p_Idx];
//             }

//             void ReserveVertices(uint64_t p_Size) override
//             {
//                 m_Positions.reserve(p_Size);
//                 m_Normals.reserve(p_Size);
//                 m_Colors.reserve(p_Size);
//             }

//             void ReserveFaces(uint64_t p_Size) override
//             {
//                 m_Indices.reserve(p_Size * 3);
//             }

//             const SVertex* GetRawVertexPointer() const override
//             {
//                 return nullptr; //m_Vertices.data();
//             }

//             const void* GetRawIndexPointer() const override
//             {
//                 return m_Indices.data();
//             }

//             bool IsFaceCountMaxReached() const override
//             {
//                 return (UINT32_MAX - m_Positions.size()) > 3;
//             }

//             void MergeSurface(ISurface *p_Surface) override
//             {
//                 ReserveVertices(p_Surface->GetVertexCount() + GetVertexCount());
//                 ReserveFaces(p_Surface->GetFaceCount() + GetFaceCount());

//                 auto other = dynamic_cast<CArraySurface*>(p_Surface);

//                 auto &otherIndices = other->m_Indices;

//                 auto startIdx = m_Positions.size();
//                 m_Positions.insert(m_Positions.end(), other->m_Positions.begin(), other->m_Positions.end());
//                 m_Normals.insert(m_Normals.end(), other->m_Normals.begin(), other->m_Normals.end());
//                 m_Colors.insert(m_Colors.end(), other->m_Colors.begin(), other->m_Colors.end());

//                 for (auto &&i : otherIndices)
//                     m_Indices.push_back(startIdx + i);

//                 p_Surface->ClaimVertices();
//             }

//             void ClaimVertices() override
//             {
//                 m_Positions.clear();
//                 m_Normals.clear();
//                 m_Colors.clear();
//                 m_Indices.clear();
//             }
//         protected:
//             const void *GetUnderlyingVertexReference() const override
//             {
//                 return nullptr; //&m_Vertices;
//             }

//             const void *GetUnderlyingIndexReference() const override
//             {
//                 return &m_Indices;
//             }

// #ifdef VCORE_RTTI_ENABLED
//             std::type_index GetUnderlyingVertexType() const override
//             {
//                 return typeid(int);
//             }

//             std::type_index GetUnderlyingIndexType() const override
//             {
//                 return typeid(m_Indices);
//             }
// #endif
//         private:
//             fast_vector<Math::Vec3f> m_Positions;
//             fast_vector<Math::Vec3f> m_Normals;
//             fast_vector<uint32_t> m_Colors;
//             // VertexArray m_Vertices;
//             fast_vector<uint32_t> m_Indices;
//     };

    using SurfaceFactory = ISurface* (*)();
    using DefaultSurface = TSurface<fast_vector<SVertex*>, fast_vector<uint32_t>, UINT32_MAX>;
} // namespace VCore

#endif