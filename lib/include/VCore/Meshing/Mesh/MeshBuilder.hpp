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

#include <map>
#include <memory>
#include <vector>
#include <VCore/Formats/IVoxelFormat.hpp>
#include <VCore/Meshing/Mesh/Mesh.hpp>
#include <VCore/Voxel/VoxelTextureMap.hpp>

namespace VCore
{
    class CMeshBuilder
    {
        public:
            CMeshBuilder(SurfaceFactory _Factory) : m_TextureMap(nullptr), m_SurfaceFactory(_Factory) {}

            /**
             * @brief Sets the texturing map.
             */
            inline void SetTextureMap(CVoxelTextureMap *_Map)
            {
                m_TextureMap = _Map;
            }

            /**
             * @brief Adds all needed textures to the mesh. This must be called before AddFace
             * 
             * @param _textures: Textures of the mesh.
             */
            void AddTextures(const ankerl::unordered_dense::map<TextureType, Texture> &_textures);

            /**
             * @brief Adds a new quad to the mesh.
             * 
             * @param _v1: Left top
             * @param _v2: Right top
             * @param _v3: Right bottom
             * @param _v4: Left bottom
             * @param _normal: Face normal
             * @param _color: Color index of the face
             * @param _material: Material of the face
             * 
             * @throws CMeshBuilderException If AddTextures has not been previously called.
             */
            void AddFace(Math::Vec3f _v1, Math::Vec3f _v2, Math::Vec3f _v3, Math::Vec3f _v4, Math::Vec3f _normal, int _color, Material _material);

            /**
             * @brief Adds a new triangle to the mesh.
             */
            void AddFace(SVertex v1, SVertex v2, SVertex v3, Material _material);

            /**
             * @brief Merges a list of meshes into one.
             * @return Returns the _MergeInto mesh or a new one, if _MergeInto is null. 
             */
            Mesh Merge(Mesh _MergeInto, const std::vector<Mesh> &_Meshes, bool _ApplyModelMatrix = false);

            /**
             * @brief Generates the new mesh.
             */
            Mesh Build();

            ~CMeshBuilder() = default;
        private:
            Material FaceMaterial;

            struct SIndexedSurface
            {
                SIndexedSurface(ISurface *_Surface) : Surface(_Surface)
                { }

                SIndexedSurface(const SIndexedSurface&) = default;
                SIndexedSurface(SIndexedSurface &&) = default;

                SIndexedSurface &operator=(const SIndexedSurface&) = default;
                SIndexedSurface &operator=(SIndexedSurface &&) = default;

                ankerl::unordered_dense::map<SVertex, int, VertexHasher> Index;
                ISurface *Surface;
            };

            int AddVertex(const SVertex &_Vertex, SIndexedSurface &_Surface);
            uint32_t AddMergeVertex(const SVertex &_Vertex, SIndexedSurface &_Surface, ankerl::unordered_dense::map<SVertex, int, VertexHasher> &_Index);

            bool IsOnBorder(const Math::Vec3f &_Pos);

            void MergeIntoThis(Mesh m, bool _ApplyModelMatrix);
            void GenerateCache(Mesh _MergeInto);

            const ankerl::unordered_dense::map<TextureType, Texture> *m_Textures;
            ankerl::unordered_dense::map<uintptr_t, SIndexedSurface> m_Surfaces;
            Mesh m_MergerMesh;

            CVoxelTextureMap *m_TextureMap;
            SurfaceFactory m_SurfaceFactory;
    };
}


#endif //MESHBUILDER_HPP