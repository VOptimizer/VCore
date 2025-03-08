/*
 * MIT License
 *
 * Copyright (c) 2022 Christian Tost
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

#ifndef MESHBUILDER_HPP
#define MESHBUILDER_HPP

#include <cstdint>
#include <vector>
#include <VCore/Formats/IVoxelFormat.hpp>
#include <VCore/Meshing/Mesh/Mesh.hpp>

namespace VCore
{
    class CMeshBuilder
    {
        public:
            CMeshBuilder(SurfaceFactory p_Factory) : m_SurfaceFactory(p_Factory) {}

            // inline const ankerl::unordered_dense::map<TextureType, Texture> *GetTextures() const
            // {
            //     return m_Textures;
            // } 

            /**
             * @brief Adds all needed textures to the mesh. This must be called before AddFace
             * 
             * @param _textures: Textures of the mesh.
             */
            void AddTextures(const ankerl::unordered_dense::map<TextureType, Texture> &p_Textures);

            /**
             * @brief Merges a list of meshes into one.
             * @return Returns the _MergeInto mesh or a new one, if _MergeInto is null. 
             */
            Mesh Merge(Mesh p_MergeInto, const std::vector<Mesh> &p_Meshes, bool p_ApplyModelMatrix = false);

            /**
             * @brief Generates the new mesh.
             */
            Mesh Build();

            void SelectSurface(const uint8_t p_MaterialHandle);

            /**
             * @brief Adds a new vertex to the currently selected surface.
             * @param _Vertex: New vertex to add.
             * @return Returns a new unique id for the new vertex.
             */
            uint32_t AddVertex(const SVertex* p_Vertex);

            void AddFace(uint32_t p_Idx1, uint32_t p_Idx2, uint32_t p_Idx3, uint32_t p_Idx4);

            ~CMeshBuilder() = default;
        private:
            Material m_FaceMaterial;
            ISurface *m_CurrentSurface;

            struct SIndexedSurface
            {
                SIndexedSurface(ISurface *p_Surface) : Surface(p_Surface)
                { }

                SIndexedSurface(const SIndexedSurface&) = default;
                SIndexedSurface(SIndexedSurface &&) = default;

                SIndexedSurface &operator=(const SIndexedSurface&) = default;
                SIndexedSurface &operator=(SIndexedSurface &&) = default;

                ankerl::unordered_dense::map<SVertex, int, VertexHasher> Index;
                ankerl::unordered_dense::map<Math::Vec3i, ankerl::unordered_dense::map<SVertex, int, VertexHasher>, Math::Vec3iHasher> Index2;
                ISurface *Surface;
            };

            int AddVertex(const SVertex &p_Vertex, SIndexedSurface &p_Surface);
            uint32_t AddMergeVertex(const SVertex &p_Vertex, SIndexedSurface &p_Surface, ankerl::unordered_dense::map<SVertex, int, VertexHasher> &p_Index);

            bool IsOnBorder(const Math::Vec3f &p_Pos);

            void MergeIntoThis(Mesh p_Mesh, bool p_ApplyModelMatrix);
            void GenerateCache(Mesh p_MergeInto);

            const ankerl::unordered_dense::map<TextureType, Texture> *m_Textures;
            ankerl::unordered_dense::map<uint8_t, SIndexedSurface> m_Surfaces;
            Mesh m_MergerMesh;

            SurfaceFactory m_SurfaceFactory;
    };
}


#endif //MESHBUILDER_HPP