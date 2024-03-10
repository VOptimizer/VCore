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

#include <VCore/Meshing/Material.hpp>
#include <VCore/Voxel/VoxelModel.hpp>
#include <VCore/Voxel/VoxelAnimation.hpp>
#include <VCore/Meshing/Mesh.hpp>

namespace VCore
{
    class IMesher;
    using Mesher = std::shared_ptr<IMesher>;

    enum class MesherTypes
    {
        SIMPLE,
        GREEDY,
        MARCHING_CUBES,
        GREEDY_CHUNKED,   //!< Old legacy greedy mesher, which looks very chunky.
        GREEDY_TEXTURED,
    };

    struct SMeshChunk : public SChunkMeta
    {
        Mesh MeshData;          //!< Mesh of the voxel model.
    };

    class IMesher
    {
        public:
            IMesher() : m_Frustum(nullptr) {}

            /**
             * @brief Creates a new mesher instance.
             */
            static Mesher Create(MesherTypes type);

            /**
             * @brief Generates the scene
             */
            std::vector<Mesh> GenerateScene(SceneNode sceneTree, bool mergeChilds = false);

            /**
             * @return Returns a list of all frames, of the animation.
             */
            std::vector<Mesh> GenerateAnimation(VoxelAnimation _Anim);

            /**
             * @return Returns the voxel mesh as triangulated vertices mesh.
             */
            Mesh GenerateMesh(VoxelModel m);

            /**
             * @brief Sets a frustum, for culling.
             */
            void SetFrustum(const CFrustum *_Frustum);

            /**
             * @brief Generates list of meshed chunks.
             * 
             * @param _Mesh: Voxel mesh to meshify.
             * @param _OnlyDirty: Meshes only dirty chunks.
             * @param _ChunkCount: Count of chunks to meshify.
             */
            virtual std::vector<SMeshChunk> GenerateChunks(VoxelModel _Mesh, bool _OnlyDirty = false);

            virtual ~IMesher();
        protected:
            /// @brief Called inside ::GenerateChunks for every chunk using multiple threads.
            /// @param _Model: The VoxelModel which is currently processed.
            /// @param _Chunk: Assigned chunk data which needs to be meshed.
            /// @param Deprecated
            /// @return Returns the _Chunk + its generated mesh.
            virtual SMeshChunk GenerateMeshChunk(VoxelModel, const SChunkMeta&, bool) { return {}; }

            std::vector<Mesh> GenerateScene(SceneNode sceneTree, Math::Mat4x4 modelMatrix, bool mergeChilds = false);
            CFrustum *m_Frustum;
    };
}


#endif //IMESHER_HPP