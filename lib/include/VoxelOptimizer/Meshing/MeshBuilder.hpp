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
#include <VoxelOptimizer/Formats/IVoxelFormat.hpp>
#include <VoxelOptimizer/Meshing/Mesh.hpp>
#include <VoxelOptimizer/Memory/Allocator.hpp>

namespace VoxelOptimizer
{
    struct SVertex
    {
        Math::Vec3f Pos;
        Math::Vec3f UV;
        Math::Vec3f Normal;
        Material Mat;
        int MaterialIndex;
    };

    class CMeshBuilder
    {
        public:
            CMeshBuilder();

            /**
             * @brief Adds all needed textures to the mesh. This must be called before AddFace
             * 
             * @param _textures: Textures of the mesh.
             */
            void AddTextures(const std::map<TextureType, Texture> &_textures);

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
            void AddFace(SVertex v1, SVertex v2, SVertex v3);

            /**
             * @brief Merges a list of meshes into one.
             */
            void Merge(Mesh mergeInto, const std::vector<Mesh> &meshes);

            /**
             * @brief Generates the new mesh.
             */
            Mesh Build();

            static Mesh Copy(Mesh mesh);

            ~CMeshBuilder() = default;
        private:
            using VectorIndexMap = VectorMap<int>; //std::map<Math::Vec3f, int, std::less<Math::Vec3f>, CAllocator<Math::Vec3f>>;
            using MaterialFacesMap = std::map<Material, GroupedFaces, std::less<Material>, CAllocator<Material>>;

            int AddPosition(Math::Vec3f _pos);
            int AddNormal(Math::Vec3f _normal);
            int AddUV(Math::Vec3f _uv);

            void MergeIntoThis(Mesh m);
            void GenerateCache();

            VectorIndexMap m_Index;
            VectorIndexMap m_NormalIndex;
            VectorIndexMap m_UVIndex;
            MaterialFacesMap m_FacesIndex;

            Mesh m_CurrentMesh;
    };
}


#endif //MESHBUILDER_HPP