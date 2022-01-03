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

#ifndef SCENENODE_HPP
#define SCENENODE_HPP

#include <VoxelOptimizer/Loaders/VoxelMesh.hpp>

namespace VoxelOptimizer
{
    class CSceneNode;
    using SceneNode = std::shared_ptr<CSceneNode>;

    class CSceneNode
    {
        public:
            using SceneNodes = std::list<SceneNode>;

            CSceneNode() : m_Scale(1, 1, 1), m_Parent(nullptr) {}

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

            SceneNodes::iterator begin()
            {
                return m_Childs.begin();
            }

            SceneNodes::iterator end()
            {
                return m_Childs.end();
            }

            SceneNodes::const_iterator begin() const
            {
                return m_Childs.begin();
            }

            SceneNodes::const_iterator end() const
            {
                return m_Childs.end();
            }

            void AddChild(SceneNode node)
            {
                node->m_Parent = this;
                m_Childs.push_back(node);
            }

            CSceneNode *Parent()
            {
                return m_Parent;
            }
        private:
            CVector m_Position;
            CVector m_Rotation;
            CVector m_Scale;

            VoxelMesh m_Mesh;
            CSceneNode* m_Parent;
            SceneNodes m_Childs;
    };
} // namespace VoxelOptimizer


#endif //SCENENODE_HPP