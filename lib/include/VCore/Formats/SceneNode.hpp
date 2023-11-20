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

#include <VCore/Voxel/VoxelModel.hpp>

namespace VCore
{
    class CSceneNode;
    using SceneNode = std::shared_ptr<CSceneNode>;

    class CSceneNode
    {
        public:
            using SceneNodes = std::vector<SceneNode>;

            CSceneNode() : Visible(true), m_Scale(1, 1, 1), m_Parent(nullptr) {}

            bool Visible;

            inline Math::Vec3f GetPosition() const
            {
                return m_Position;
            }
            
            inline void SetPosition(const Math::Vec3f &position)
            {
                m_Position = position;
            }

            inline Math::Vec3f GetRotation() const
            {
                return m_Rotation;
            }
            
            inline void SetRotation(const Math::Vec3f &rotation)
            {
                m_Rotation = rotation;
            }

            inline Math::Vec3f GetScale() const
            {
                return m_Scale;
            }
            
            inline void SetScale(const Math::Vec3f &scale)
            {
                m_Scale = scale;
            }

            inline VoxelModel GetMesh() const
            {
                return m_Mesh;
            }
            
            inline void SetMesh(VoxelModel mesh)
            {
                m_Mesh = mesh;
            }

            inline std::string GetName() const
            {
                return m_Name;
            }
            
            inline void SetName(const std::string &name)
            {
                m_Name = name;
            }

            inline Math::Mat4x4 ModelMatrix() const
            {
                Math::Mat4x4 mm; //* Math::Mat4x4::Rotation(m_Rotation); // * Math::Mat4x4::Scale(m_Scale);
                mm
                    .Rotate(Math::Vec3f(0, 0, 1), m_Rotation.z)
                    .Rotate(Math::Vec3f(1, 0, 0), m_Rotation.x)
                    .Rotate(Math::Vec3f(0, 1, 0), m_Rotation.y);

                mm *= Math::Mat4x4::Scale(m_Scale);
                return Math::Mat4x4::Translation(m_Position) * mm;
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

            CSceneNode *GetParent()
            {
                return m_Parent;
            }

            uint32_t ChildCount() const
            {
                return m_Childs.size();
            }
        private:
            Math::Vec3f m_Position;
            Math::Vec3f m_Rotation;
            Math::Vec3f m_Scale;

            VoxelModel m_Mesh;
            CSceneNode* m_Parent;
            SceneNodes m_Childs;
            std::string m_Name;
    };
}


#endif //SCENENODE_HPP