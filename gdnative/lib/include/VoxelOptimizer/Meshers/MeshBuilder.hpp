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
#include <VoxelOptimizer/Loaders/ILoader.hpp>
#include <VoxelOptimizer/Mesh.hpp>

namespace VoxelOptimizer
{
    struct SVertex
    {
        CVector Pos;
        CVector UV;
        CVector Normal;
        int Material;
    };

    class CMeshBuilder
    {
        public:
            CMeshBuilder();

            /**
             * @brief Sets the loader to use to generate the mesh.
             */
            void UseLoader(Loader loader)
            {
                m_Loader = loader;
            }

            /**
             * @brief Adds a new quad to the mesh.
             * 
             * @param Color: Colorindex of the face
             * @param Material: Materialindex of the face
             */
            void AddFace(CVector v1, CVector v2, CVector v3, CVector v4, CVector Normal, int Color, int Material);

            /**
             * @brief Adds a new triangle to the mesh.
             */
            void AddFace(SVertex v1, SVertex v2, SVertex v3);

            /**
             * @brief Merges a list of meshes into one.
             */
            void Merge(const std::vector<Mesh> &meshes);

            /**
             * @brief Generates the new mesh.
             */
            Mesh Build();

            static Mesh Copy(Mesh mesh);

            ~CMeshBuilder() = default;
        private:
            int AddVertex(CVector Vertex);
            int AddNormal(CVector Normal);
            int AddUV(CVector UV);

            std::map<CVector, int> m_Index;
            std::map<CVector, int> m_NormalIndex;
            std::map<CVector, int> m_UVIndex;
            std::map<int, GroupedFaces> m_FacesIndex;

            Mesh m_CurrentMesh;
            Loader m_Loader;
    };
} // namespace VoxelOptimizer


#endif //MESHBUILDER_HPP