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

#ifndef IVOXELFORMAT_HPP
#define IVOXELFORMAT_HPP

#include <string>
#include <vector>
#include <VoxelOptimizer/Loaders/ILoader.hpp>
#include <VoxelOptimizer/Loaders/VoxelMesh.hpp>

namespace VoxelOptimizer
{
    class CSceneNode;
    using SceneNode = std::shared_ptr<CSceneNode>;

    class CSceneNode
    {
        public:
            CSceneNode() = default;

            inline CVector Position() const
            {
                return m_Position;
            }
            
            inline void Position(const CVector &position)
            {
                m_Position = position;
            }

            inline CVector Rotation() const
            {
                return m_Rotation;
            }
            
            inline void Rotation(const CVector &rotation)
            {
                m_Position = rotation;
            }

            inline CVector Scale() const
            {
                return m_Scale;
            }
            
            inline void Scale(const CVector &scale)
            {
                m_Scale = scale;
            }

            inline VoxelMesh Mesh() const
            {
                return m_Mesh;
            }
            
            inline void Mesh(VoxelMesh mesh)
            {
                m_Mesh = mesh;
            }

            inline CMat4x4 ModelMatrix() const
            {
                return CMat4x4::Translation(m_Position) * CMat4x4::Rotation(m_Rotation) * CMat4x4::Scale(m_Scale);
            }

            auto begin()
            {
                return m_Childs.begin();
            }

            auto end()
            {
                return m_Childs.begin();
            }

            auto begin() const
            {
                return m_Childs.begin();
            }

            auto end() const
            {
                return m_Childs.begin();
            }
        private:
            CVector m_Position;
            CVector m_Rotation;
            CVector m_Scale;

            VoxelMesh m_Mesh;
            std::list<SceneNode> m_Childs;
    };

    class IVoxelFormat;
    using VoxelFormat = std::shared_ptr<IVoxelFormat>;

    class IVoxelFormat : public ILoader
    {
        public:
            void UseLoader(Loader loader)
            {
                m_Materials = loader->GetMaterials();
                m_Textures = loader->GetTextures();
            }

            /**
             * @brief Creates an instance of a the given loader;
             */
            static VoxelFormat Create(LoaderTypes type);

            /**
             * @brief Saves the voxel mesh as a voxel file.
             */         
            void Save(const std::string &path, const std::vector<VoxelMesh> &meshes);

            /**
             * @brief Saves the voxel mesh as a voxel file.
             * 
             * @return Returns the file as memory stream.
             */         
            virtual std::vector<char> Save(const std::vector<VoxelMesh> &meshes);

            /**
             * @return Gets the scene tree of this file.
             */
            inline SceneNode SceneTree() const
            {
                return m_SceneTree;
            }

            /**
             * @brief Sets the scene tree.
             */
            inline SceneNode SceneTree(SceneNode tree)
            {
                m_SceneTree = tree;
            }
        protected:
            SceneNode m_SceneTree;
    };
} // namespace VoxelOptimizer


#endif //IVOXELFORMAT_HPP