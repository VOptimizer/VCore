/*
 * MIT License
 *
 * Copyright (c) 2021 Christian Tost
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

#ifndef IMESHER_HPP
#define IMESHER_HPP

#include <map>
#include <list>

#include <VoxelOptimizer/Meshing/Material.hpp>
#include <VoxelOptimizer/Voxel/VoxelMesh.hpp>

#include <VoxelOptimizer/Meshing/Mesh.hpp>

namespace VoxelOptimizer
{
    class IMesher;
    using Mesher = std::shared_ptr<IMesher>;

    enum class MesherTypes
    {
        SIMPLE,
        GREEDY,
        MARCHING_CUBES,
        FLOOD
    };

    class IMesher
    {
        public:
            IMesher() = default;

            /**
             * @brief Creates a new mesher instance.
             */
            static Mesher Create(MesherTypes type);

            /**
             * @brief Generates the scene
             */
            std::list<Mesh> GenerateScene(SceneNode sceneTree, bool mergeChilds = false);

            /**
             * @return Returns the voxel mesh as triangulated vertices mesh.
             */
            Mesh GenerateMesh(VoxelMesh m);

            /**
             * @brief Generates list of meshed chunks.
             * 
             * @param m: Voxel mesh to meshify.
             * @param onlyDirty: Meshes only dirty chunks.
             */
            virtual std::map<CVector, Mesh> GenerateMeshes(VoxelMesh m, bool onlyDirty = false) = 0;

            virtual ~IMesher() = default;
        protected:
            std::list<Mesh> GenerateScene(SceneNode sceneTree, CMat4x4 modelMatrix, bool mergeChilds = false);
            std::vector<Material> m_CurrentUsedMaterials;
            std::map<CVectori, Voxel> m_Voxels;
    };
} // namespace VoxelOptimizer


#endif //IMESHER_HPP