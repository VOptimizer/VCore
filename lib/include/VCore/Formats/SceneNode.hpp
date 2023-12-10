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
#include <VCore/Voxel/VoxelAnimation.hpp>

namespace VCore
{
    class CSceneNode;
    using SceneNode = std::shared_ptr<CSceneNode>;

    class CSceneNode
    {
        public:
            using SceneNodes = std::vector<SceneNode>;

            CSceneNode() : Visible(true), Scale(1, 1, 1), m_Parent(nullptr) {}

            bool Visible;
            Math::Vec3f Position;
            Math::Vec3f Rotation;
            Math::Vec3f Scale;
            std::string Name;

            VoxelModel Mesh;            //!< A scene node can either have a mesh or an animation
            VoxelAnimation Animation;   //!< A scene node can either have a mesh or an animation

            inline Math::Mat4x4 GetModelMatrix() const
            {
                Math::Mat4x4 mm;
                mm
                    .Rotate(Math::Vec3f(0, 0, 1), Rotation.z)
                    .Rotate(Math::Vec3f(1, 0, 0), Rotation.x)
                    .Rotate(Math::Vec3f(0, 1, 0), Rotation.y);

                mm *= Math::Mat4x4::Scale(Scale);
                return Math::Mat4x4::Translation(Position) * mm;
            }

            SceneNodes::iterator begin()
            {
                return m_Children.begin();
            }

            SceneNodes::iterator end()
            {
                return m_Children.end();
            }

            SceneNodes::const_iterator begin() const
            {
                return m_Children.begin();
            }

            SceneNodes::const_iterator end() const
            {
                return m_Children.end();
            }

            void AddChild(SceneNode node)
            {
                node->m_Parent = this;
                m_Children.push_back(node);
            }

            CSceneNode *GetParent()
            {
                return m_Parent;
            }

            uint32_t GetChildrenCount() const
            {
                return m_Children.size();
            }
        private:
            CSceneNode* m_Parent;
            SceneNodes m_Children;
    };
}


#endif //SCENENODE_HPP