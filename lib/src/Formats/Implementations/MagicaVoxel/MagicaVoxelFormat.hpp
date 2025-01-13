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

namespace VCore
{
    constexpr static int PALETTE_SIZE = 256;

    enum NodeType
    {
        TRANSFORM,
        GROUP,
        SHAPE
    };

    struct SNode
    {
        public:
            SNode() = default;
            SNode(NodeType type) : Type(type) {} 

            NodeType Type;

            int NodeID;
            ankerl::unordered_dense::map<std::string, std::string> Attributes;
    };

    struct SFrameTransform
    {
        Math::Vec3i Translation;
        Math::Vec3f Rotation;
        int FrameIdx;
    };

    struct STransformNode : public SNode
    {
        public:
            STransformNode() : SNode(NodeType::TRANSFORM) {}

            int LayerID;
            int NumFrames;

            std::string Name;

            int ChildID;
            fast_vector<SFrameTransform> Frames;
    };

    struct SGroupNode : public SNode
    {
        public:
            SGroupNode() : SNode(NodeType::GROUP), ChildIdx(0) {}

            int ChildIdx;

            fast_vector<int> ChildrensID;
    };

    struct SFrame
    {
        int ModelId;
        int FrameIdx;
    };

    struct SShapeNode : public SNode
    {
        public:
            SShapeNode() : SNode(NodeType::SHAPE) {}

            fast_vector<SFrame> Models;
    };

    using Node = std::shared_ptr<SNode>;
    using TransformNode = std::shared_ptr<STransformNode>;
    using GroupNode = std::shared_ptr<SGroupNode>;
    using ShapeNode = std::shared_ptr<SShapeNode>;

    class CMagicaVoxelFormat : public IVoxelFormat
    {
        public:  
            CMagicaVoxelFormat() = default;
            ~CMagicaVoxelFormat() = default;

        protected:
            void ParseFormat() override;
            void WriteFormat() override;

            void ClearCache() override;

        private:
            struct SChunkHeader
            {
                char ID[4];
                int ChunkContentSize;
                int ChildChunkSize;
            };

            /** @brief Loads the default color palette into m_ColorPalette */
            void LoadDefaultPalette();

            Math::Vec3i ProcessSize();
            void ProcessXYZI(VoxelModel m, const Math::Vec3i &_Size);
            fast_vector<fast_vector<SFrame>> ProcessMaterialAndSceneGraph();

            TransformNode ProcessTransformNode();
            GroupNode ProcessGroupNode();
            ShapeNode ProcessShapeNode();

            void SkipDict();

            int WriteAnimation(const VoxelAnimation &_Animation);
            int WriteModel(const VoxelModel &_Model, ankerl::unordered_dense::map<Math::Vec3i, ShapeNode, Math::Vec3iHasher> *_Shapes = nullptr, uint32_t _FrameIdx = 0);

            void TraverseVCoreSceneTree();
            int TraverseSceneTreeNode(const SceneNode &_Node);
            void WriteSceneTree();

            /**
             * @brief Writes a string.
             * A string is stored without null termination and has an int32 as "prefix" which contains the size of the string.
             */
            void WriteString(const char *_String);
            
            void WriteDictKeyString(const char *_Key, const char *_Value);
            void WriteDictKeyInt(const char *_Key, const int _Value);
            void WriteDictKeyFloat(const char *_Key, const float _Value);
            void WriteDictKeyVec3i(const char *_Key, const Math::Vec3i &_Value);

            // Scenetree nodes for the voxel file
            fast_vector<Node> m_MagicaSceneTree;

            // Maps
            ankerl::unordered_dense::map<uint32_t, uint32_t> m_ColorMapping;
            ankerl::unordered_dense::map<uint32_t, uint32_t> m_MaterialMapping;
            ankerl::unordered_dense::map<int, SceneNode> m_ModelSceneTreeMapping;
            ankerl::unordered_dense::map<uint32_t, uint8_t> m_VoxelIndexMap;

            ankerl::unordered_dense::set<uintptr_t> m_AlreadyWrittenModels;
            ankerl::unordered_dense::set<uintptr_t> m_AlreadyWrittenAnimations;

            CColor m_ColorPalette[PALETTE_SIZE];

            // Index counters
            uint8_t m_VoxelIndex;
            uint32_t m_ModelCounter;
            size_t m_UsedColorsPos;

            bool m_HasEmission;
    };
}


#endif //VOXELFORMAT_HPP