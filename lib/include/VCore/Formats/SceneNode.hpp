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

#include <VCore/Misc/MessageBus.hpp>
#include <VCore/Voxel/Frustum.hpp>
#include <VCore/Voxel/Storage/VoxelSpace.hpp>
#include <VCore/Math/Vector.hpp>
#include <VCore/Misc/fast_vector.hpp>
#include <VCore/Voxel/VoxelModel.hpp>

#include <VCore/Meshing/Mesh/Mesh.hpp>
#include <VCore/Math/Mat4x4.hpp>
#include <VCore/Voxel/BBox.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

namespace VCore
{
    using BBoxMessageBus = TMessageBus<uint64_t, CBBox>;

    class CVoxelSceneTree;
    class CSceneNodeBase
    {
        friend CVoxelSceneTree;
        public:
            using Children = fast_vector<CSceneNodeBase*>;

            CSceneNodeBase(CSceneNodeBase *p_Parent) : m_Parent(p_Parent) {}

            std::string Name;
            bool Visible{true};

            inline void SetPosition(const Math::Vec3f &p_Position)
            {
                m_Position = p_Position;
                NotifyChildrenTranformDirty();
            }

            inline void SetRotation(const Math::Vec3f &p_Rotation)
            {
                m_Rotation = p_Rotation;
                NotifyChildrenTranformDirty();
            }

            inline void SetScale(const Math::Vec3f &p_Scale)
            {
                m_Scale = p_Scale;
                NotifyChildrenTranformDirty();
            }

            inline void SetParent(CSceneNodeBase *p_Parent)
            {
                if(!p_Parent)
                    return;

                // if(m_Parent)
                //     m_Parent->Remove(this);
                m_Parent = p_Parent;
            }

            inline Math::Vec3f GetPosition() const { return m_Position; }
            inline Math::Vec3f GetRotation() const { return m_Rotation; }
            inline Math::Vec3f GetScale() const { return m_Scale; }
            inline CSceneNodeBase *GetParent() const { return m_Parent; }

            /** Foreach accessors. */

            virtual Children::iterator begin() = 0;
            virtual Children::iterator end() = 0;

            virtual Children::const_iterator begin() const = 0;
            virtual Children::const_iterator end() const = 0;

            /**
             * @brief Adds a new child to this node.
             * @param p_Node New node to add.
             */
            virtual void AddChild(CSceneNodeBase *p_Node) = 0;

            /** @return Gets the current children count of this node. */
            virtual uint32_t GetChildrenCount() const = 0;

            /** @return Gets the global transform of this node. */
            inline Math::Mat4x4 GetGlobalTransform() const
            {
                if(m_TransformDirty) // Is the transformation dirty?
                {
                    m_TransformDirty = false;

                    // Update cache
                    if(m_Parent)
                        m_GlobalTransform = m_Parent->GetGlobalTransform() * GetLocalTransform();
                    else
                        m_GlobalTransform = GetLocalTransform();
                }

                return m_GlobalTransform;
            }

            inline Math::Mat4x4 GetLocalTransform() const
            {
                Math::Mat4x4 localTransform;
                localTransform
                    .Rotate(Math::Vec3f(0, 0, 1), m_Rotation.z)
                    .Rotate(Math::Vec3f(1, 0, 0), m_Rotation.x)
                    .Rotate(Math::Vec3f(0, 1, 0), m_Rotation.y);

                localTransform *= Math::Mat4x4::Scale(m_Scale);
                return Math::Mat4x4::Translation(m_Position) * localTransform;
            }

            virtual ~CSceneNodeBase() = default;
        protected:
            void NotifyChildrenTranformDirty()
            {
                m_TransformDirty = true;
                for (auto &&child : *this) 
                    child->NotifyChildrenTranformDirty();
            }

            virtual void NotifyTreeChanged(CSceneNodeBase *p_Node, bool p_Added)
            {
                if(m_Parent)
                    m_Parent->NotifyTreeChanged(p_Node, p_Added);
            }

            virtual void SetMessageBus(BBoxMessageBus *p_Bus)
            {
                for (auto &&node : *this)
                    node->SetMessageBus(p_Bus);
            }

            void CalculateBBox()
            {
                m_BBox = CBBox(Math::Vec3i(INT32_MAX, INT32_MAX, INT32_MAX), Math::Vec3i());
                for (auto &&child : *this) 
                {
                    m_BBox.Beg = m_BBox.Beg.min(child->m_BBox.Beg);
                    m_BBox.End = m_BBox.End.max(child->m_BBox.End);
                }
            }

            void NotifyBBoxChanged()
            {
                if(m_Parent)
                {
                    m_Parent->CalculateBBox();
                    m_Parent->NotifyBBoxChanged();
                }
            }

            virtual void DoFrustumCulling(const CFrustum &p_Frustum, fast_vector<CSceneNodeBase*> &p_Models)
            {
                for (auto &&child : *this) 
                {
                    if(p_Frustum.IsOnFrustum(child->m_BBox))
                        child->DoFrustumCulling(p_Frustum, p_Models);
                }
            }

            mutable bool m_TransformDirty{true};
            Math::Vec3f m_Position;
            Math::Vec3f m_Rotation;
            Math::Vec3f m_Scale{1,1,1};
            CBBox m_BBox;

        private:
            CSceneNodeBase *m_Parent;
            mutable Math::Mat4x4 m_GlobalTransform;
    };

    class CSceneNode : public CSceneNodeBase
    {
        public:
            CSceneNode(CSceneNodeBase *p_Parent) : CSceneNodeBase(p_Parent) {}

            /** Foreach accessors. */

            inline Children::iterator begin() override { return m_Children.begin(); }
            inline Children::iterator end() override { return m_Children.end(); }

            inline Children::const_iterator begin() const override { return m_Children.begin(); }
            inline Children::const_iterator end() const override { return m_Children.end(); }

            /**
             * @brief Adds a new child to this node.
             * @param p_Node New node to add.
             */
            inline void AddChild(CSceneNodeBase *p_Node) override 
            { 
                p_Node->SetParent(this); 
                m_Children.push_back(p_Node);
                NotifyTreeChanged(p_Node, true);
            }

            /** @return Gets the current children count of this node. */
            inline uint32_t GetChildrenCount() const override { return m_Children.size(); }

            virtual ~CSceneNode() override
            {
                for (auto &&child : m_Children)
                    delete child;
            }
        private:
            Children m_Children;
    };

    class CSceneModelNode : public CSceneNode, public IMessageHandler<uint64_t, CBBox>
    {
        friend CVoxelSceneTree;
        public:
            CSceneModelNode(CSceneNodeBase *p_Parent, const uint64_t p_ModelId) : CSceneNode(p_Parent), ModelId(p_ModelId), m_MessageBus(nullptr)
            {  }

            uint64_t ModelId;

            void OnMessage(const uint64_t&, const CBBox &p_NewBBox) override
            {
                CalculateBBox();
                m_BBox.Beg = m_BBox.Beg.min(GetGlobalTransform() * p_NewBBox.Beg);
                m_BBox.End = m_BBox.End.max(GetGlobalTransform() * p_NewBBox.End);
                NotifyBBoxChanged();
            }

            ~CSceneModelNode() override
            { Unsubscribe(); }

        protected:
            void DoFrustumCulling(const CFrustum &p_Frustum, fast_vector<CSceneNodeBase*> &p_Models) override
            {
                p_Models.push_back(this);
                CSceneNodeBase::DoFrustumCulling(p_Frustum, p_Models);
            }

            void SetMessageBus(BBoxMessageBus *p_Bus) override
            {
                Unsubscribe();
                m_MessageBus = p_Bus;
                m_MessageBus->AddHandler(ModelId, this);
            }

            void Unsubscribe()
            {
                if(m_MessageBus)
                    m_MessageBus->RemoveHandler(ModelId, this);
            }

            BBoxMessageBus *m_MessageBus;
    };

    class CSceneAnimationNode : public CSceneNode
    {
        public:
            struct SFrame
            {
                uint32_t ModelId;
                uint32_t FrameIdx;
            };

            CSceneAnimationNode(CSceneNodeBase *p_Parent, fast_vector<SFrame> &&p_Frames) : CSceneNode(p_Parent), Frames(std::move(p_Frames)) {}

            fast_vector<SFrame> Frames;

            ~CSceneAnimationNode() override = default;

        protected:
            void DoFrustumCulling(const CFrustum &p_Frustum, fast_vector<CSceneNodeBase*> &p_Models) override
            {
                p_Models.push_back(this);
                CSceneNodeBase::DoFrustumCulling(p_Frustum, p_Models);
            }
    };

    /** Root of a scene tree */
    template<class T>
    class TSceneTree : public CSceneNodeBase
    {
        public:
            TSceneTree() : CSceneNodeBase(nullptr) {}
            TSceneTree(std::shared_ptr<CSceneNode::Children> p_Children) : CSceneNodeBase(nullptr), m_Children(p_Children) {}

            /** Foreach accessors. */

            inline CSceneNode::Children::iterator begin() override 
            { 
                if(!m_Children)
                    return nullptr;

                return m_Children->begin(); 
            }
            
            inline CSceneNode::Children::iterator end() override 
            { 
                if(!m_Children)
                    return nullptr;

                return m_Children->end(); 
            }

            inline CSceneNode::Children::const_iterator begin() const override 
            { 
                if(!m_Children)
                    return nullptr;

                return m_Children->begin(); 
            }

            inline CSceneNode::Children::const_iterator end() const override
            { 
                if(!m_Children)
                    return nullptr;
                
                return m_Children->end(); 
            }

            /**
             * @brief Adds a new child to this scene tree.
             * @param p_Node New node to add.
             */
            inline void AddChild(CSceneNodeBase *p_Node) override
            { 
                if(p_Node == this) [[unlikely]]
                    return;

                if(!m_Children)
                    m_Children = std::make_shared<CSceneNode::Children>();

                p_Node->SetParent(this);
                m_Children->push_back(p_Node); 
                NotifyTreeChanged(p_Node, true);
            }

            /** @brief Adds a new model to this scene tree */
            inline virtual void AddModel(T p_Model) { m_Models.push_back(p_Model); }

            const fast_vector<T> GetModels() const { return m_Models; }

            /** @return Gets the current children count of this node. */
            inline uint32_t GetChildrenCount() const override { return m_Children->size(); }

            inline const std::shared_ptr<CSceneNode::Children> GetChildren() const
            {
                return m_Children;
            }

            ~TSceneTree() override = default;
        protected:
            struct ChildrenDeleter
            {
                void operator()(CSceneNode::Children* p_Children) const
                {
                    for (auto &&child : *p_Children)
                        delete child;

                    delete p_Children;
                }
            };

            fast_vector<T> m_Models;
            std::shared_ptr<CSceneNode::Children> m_Children;
    };

    class CVoxelSceneTree : public TSceneTree<VoxelModel>, public IMessageHandler<uintptr_t, CBBox>
    {
        public:
            fast_vector<CSceneNodeBase*> DoFrustumCulling(const CFrustum &p_Frustum)
            {
                fast_vector<CSceneNodeBase*> result;

                CSceneNodeBase::DoFrustumCulling(p_Frustum, result);

                return result;
            }

            void AddModel(VoxelModel p_Model) override
            {
                TSceneTree<VoxelModel>::AddModel(p_Model);
                ModelBBoxMessageBus::GetInstance()->AddHandler((uintptr_t)p_Model.get(), this);
            }

            void OnMessage(const uintptr_t &p_Model, const CBBox &p_BBox) override
            {
                // Find the handle, and inform all subnotes
                for (uint64_t i = 0; i < m_Models.size(); i++) 
                {
                    if(p_Model == (uintptr_t)m_Models[i].get())
                    {
                        m_MessageBus.PublishMessage(i, p_BBox);
                        break;
                    }
                }                    
            }

            ~CVoxelSceneTree() override
            {
                for (auto &&model : m_Models) 
                    ModelBBoxMessageBus::GetInstance()->RemoveHandler((uintptr_t)model.get(), this);
            }
        
        protected:
            void NotifyTreeChanged(CSceneNodeBase *p_Node, bool p_Added) override
            {
                if(p_Added)
                    p_Node->SetMessageBus(&m_MessageBus);
            }

        private:
            BBoxMessageBus m_MessageBus;
    };

    template <class T>
    class ISceneTreeVisitor
    {
        public:
            ISceneTreeVisitor() = default;
            ISceneTreeVisitor(const std::shared_ptr<TSceneTree<T>> &p_SceneTree) : m_SceneTree(p_SceneTree) {}

            virtual ~ISceneTreeVisitor() = default;
        protected:
            /** Traverses the complete scene tree */
            virtual void TraverseTree() { TraverseNode(m_SceneTree.get()); }

            virtual void TraverseNode(const CSceneNodeBase *p_Node)
            {
                EnterSceneNode(p_Node);
                for (auto &&scenenode: *p_Node)
                    TraverseNode(scenenode);
                LeaveSceneNode(p_Node);
            }

            virtual void EnterSceneNode(const CSceneNodeBase *p_Node) = 0;
            virtual void LeaveSceneNode(const CSceneNodeBase *p_Node) = 0;

            std::shared_ptr<TSceneTree<T>> m_SceneTree;
    };

    using VoxelSceneTree_t = CVoxelSceneTree; //TSceneTree<VoxelModel>;
    using VoxelSceneTree = std::shared_ptr<VoxelSceneTree_t>;

    using RenderSceneTree_t = TSceneTree<Mesh>;
    using RenderSceneTree = std::shared_ptr<RenderSceneTree_t>;
}


#endif //SCENENODE_HPP