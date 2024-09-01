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

#include "Vertex.hpp"
#include <VCore/Meshing/Material.hpp>
#include <VCore/VConfig.hpp>
#include <VCore/Misc/fast_vector.hpp>
#include <climits>

#ifdef VCORE_RTTI_ENABLED
#include <stdexcept>
#include <typeinfo>
#include <typeindex>
#endif

namespace VCore
{
    class ISurface
    {
        public:
            Material FaceMaterial;          //!< Material of this surface

            /**
             * @brief Adds a new Vertex to the surface.
             * @return Returns the index of the vertex.
             */
            virtual uint32_t AddVertex(const SVertex &_Vertex) = 0;

            /**
             * @brief Adds a face to the surface. The order must be counter clockwise.
             */
            virtual void AddFace(uint32_t _Idx1, uint32_t _Idx2, uint32_t _Idx3) = 0;

            /**
             * @return Returns the vertex count, which this surface holds. 
             */
            virtual uint64_t GetVertexCount() const = 0;

            /**
             * @return Returns the face count, which this surface holds. 
             */
            virtual uint64_t GetFaceCount() const = 0;

            /**
             * @return Returns a vertex at the given index.
             */
            virtual SVertex GetVertex(uint64_t _Idx) const = 0;

            /**
             * @return Returns an index at the given index.
             */
            virtual uint32_t GetIndex(uint64_t _Idx) const = 0;

            /**
             * @brief Reserve memory for the given _Size of vertices.
             */
            virtual void ReserveVertices(uint64_t _Size) = 0;

            /**
             * @brief Reserve memory for the given _Size of faces. (_Size * 3) = index count.
             */
            virtual void ReserveFaces(uint64_t _Size) = 0;

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
            virtual void MergeSurface(const ISurface *_Surface) = 0;

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
            uint32_t AddVertex(const SVertex &_Vertex) override
            {
                m_Vertices.push_back(_Vertex);
                return m_Vertices.size() - 1;
            }

            void AddFace(uint32_t _Idx1, uint32_t _Idx2, uint32_t _Idx3) override
            {
                m_Indices.push_back(_Idx1);
                m_Indices.push_back(_Idx2);
                m_Indices.push_back(_Idx3);
            }

            uint64_t GetVertexCount() const override
            {
                return (uint64_t)m_Vertices.size();
            }

            uint64_t GetFaceCount() const override
            {
                return (uint64_t)(m_Indices.size() / 3);
            }

            SVertex GetVertex(uint64_t _Idx) const override
            {
                return m_Vertices[_Idx];
            }

            uint32_t GetIndex(uint64_t _Idx) const override
            {
                return m_Indices[_Idx];
            }

            void ReserveVertices(uint64_t _Size) override
            {
                m_Vertices.reserve(_Size);
            }

            void ReserveFaces(uint64_t _Size) override
            {
                m_Indices.reserve(_Size * 3);
            }

            const SVertex* GetRawVertexPointer() const override
            {
                return m_Vertices.data();
            }

            const void* GetRawIndexPointer() const override
            {
                return m_Indices.data();
            }

            bool IsFaceCountMaxReached() const override
            {
                return (IndexMax - m_Vertices.size()) > 3;
            }

            void MergeSurface(const ISurface *_Surface) override
            {
                ReserveVertices(_Surface->GetVertexCount() + GetVertexCount());
                ReserveFaces(_Surface->GetFaceCount() + GetFaceCount());

                auto &otherVertices = _Surface->GetVertexReference<VertexArray>();
                auto &otherIndices = _Surface->GetIndexReference<IndexArray>();

                auto startIdx = m_Vertices.size();
                m_Vertices.insert(m_Vertices.end(), otherVertices.begin(), otherVertices.end());
                for (auto &&i : otherIndices)
                    m_Indices.push_back(startIdx + i);
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

    using SurfaceFactory = ISurface* (*)();
    using DefaultSurface = TSurface<fast_vector<SVertex>, fast_vector<uint32_t>, UINT32_MAX>;
} // namespace VCore

#endif