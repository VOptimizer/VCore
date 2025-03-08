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

#ifndef VOXELFORMAT_HPP
#define VOXELFORMAT_HPP

#include <VCore/Math/Mat4x4.hpp>
#include <VCore/Formats/IVoxelFormat.hpp>
#include <VCore/Misc/unordered_dense.h>
#include <VCore/Misc/fast_vector.hpp>
#include <cstdint>
#include <string>

#include "MagicaVoxelModelParser.hpp"
#include <VCore/Formats/SceneNode.hpp>
#include <VCore/Voxel/Storage/VoxelSpace.hpp>

namespace VCore
{
    // MagicaVoxel supports 256 different colors and materials.
    constexpr static int PALETTE_SIZE = 256;

    enum class NodeType : uint8_t
    {
        TRANSFORM,
        GROUP,
        SHAPE
    };

    struct SFrameTransform
    {
        Math::Vec3i Translation;
        Math::Vec3f Rotation;
        uint32_t FrameIdx;
    };

    struct SNode
    {
        public:
            SNode() = default;
            SNode(const SNode &) = default;
            SNode(SNode &&) = default;
            SNode(NodeType p_Type) : Type(p_Type) {}

            SNode &operator=(const SNode &) = default;
            SNode &operator=(SNode &&) = default;

            NodeType Type;
            uint32_t NodeId;
    };

    struct STransformNode : public SNode
    {
        public:
            STransformNode() : SNode(NodeType::TRANSFORM) {}
            STransformNode(const STransformNode &) = default;
            STransformNode(STransformNode &&) = default;
            
            STransformNode &operator=(const STransformNode &) = default;
            STransformNode &operator=(STransformNode &&) = default;

            std::string Name;

            uint32_t ChildId;
            fast_vector<SFrameTransform> Frames;
    };

    struct SGroupNode : public SNode
    {
        public:
            SGroupNode() : SNode(NodeType::GROUP) {}
            SGroupNode(const SGroupNode &) = default;
            SGroupNode(SGroupNode &&) = default;

            SGroupNode &operator=(const SGroupNode &) = default;
            SGroupNode &operator=(SGroupNode &&) = default;

            fast_vector<uint32_t> Children;
    };

    struct SShapeNode : public SNode
    {
        public:
            SShapeNode() : SNode(NodeType::SHAPE) {}
            SShapeNode(const SShapeNode &) = default;
            SShapeNode(SShapeNode &&) = default;

            SShapeNode &operator=(const SShapeNode &) = default;
            SShapeNode &operator=(SShapeNode &&) = default;

            fast_vector<CSceneAnimationNode::SFrame> Models;
    };

    struct SMagicaVoxelChunkHeader
    {
        // char ID[4];
        uint32_t Id;
        uint32_t ChunkContentSize;
        uint32_t ChildChunkSize;
    };

    class CMagicaVoxelFormat : public IVoxelFormat
    {
        public:  
            CMagicaVoxelFormat() = default;
            ~CMagicaVoxelFormat() = default;

        protected:
            void ParseFormat() override;
            void WriteFormat() override {}

            void ClearCache() override;

        private:
            /** @brief Loads the default color palette into m_ColorPalette */
            void LoadDefaultPalette();

            void LoadColorPalette();
            void ProcessChunks();
            void ProcessMaterial();
            void ProcessModel(const std::shared_ptr<uint32_t[]> &p_Colorpalette);
            

            STransformNode ProcessTransformNode();
            SGroupNode ProcessGroupNode();
            SShapeNode ProcessShapeNode();

            void SkipDict();

            // int WriteAnimation(const VoxelAnimation &p_Animation);
            int WriteModel(const VoxelModel &p_Model, ankerl::unordered_dense::map<Math::Vec3i, SShapeNode, Math::Vec3iHasher> *p_Shapes = nullptr, uint32_t p_FrameIdx = 0);

            void TraverseVCoreSceneTree();
            int TraverseSceneTreeNode(const CSceneNode* p_Node);
            void WriteSceneTree();

            /**
             * @brief Writes a string.
             * A string is stored without null termination and has an int32 as "prefix" which contains the size of the string.
             */
            void WriteString(const char *p_String);
            
            void WriteDictKeyString(const char *p_Key, const char *p_Value);
            void WriteDictKeyInt(const char *p_Key, const int p_Value);
            void WriteDictKeyFloat(const char *p_Key, const float p_Value);
            void WriteDictKeyVec3i(const char *p_Key, const Math::Vec3i &p_Value);

            // Scenetree nodes for the voxel file
            fast_vector<SNode> m_MagicaSceneTree;

            // Maps
            ankerl::unordered_dense::map<uint32_t, uint8_t> m_VoxelIndexMap;

            ankerl::unordered_dense::set<uintptr_t> m_AlreadyWrittenModels;
            ankerl::unordered_dense::set<uintptr_t> m_AlreadyWrittenAnimations;

            std::shared_ptr<ankerl::unordered_dense::map<uint8_t, CMaterial, Uint8Hasher>> m_NotDefaultMaterials;

            CColor m_ColorPalette[PALETTE_SIZE];

            // Index counters
            uint8_t m_VoxelIndex;
            uint32_t m_ModelCounter;
    };
}


#endif //VOXELFORMAT_HPP