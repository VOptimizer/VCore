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

namespace VCore
{
    class CMagicaVoxelFormat : public IVoxelFormat
    {
        public:  
            CMagicaVoxelFormat() = default;

            ~CMagicaVoxelFormat() = default;
        private:
            void ParseFormat() override;
            void ClearCache() override;

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
                    std::map<std::string, std::string> Attributes;
            };

            struct SFrameTransform
            {
                Math::Vec3f Translation;
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
                    std::vector<SFrameTransform> Frames;
            };

            struct SGroupNode : public SNode
            {
                public:
                    SGroupNode() : SNode(NodeType::GROUP), ChildIdx(0) {}

                    int ChildIdx;

                    std::vector<int> ChildrensID;
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

                    std::vector<SFrame> Models;
            };

            using Node = std::shared_ptr<SNode>;
            using TransformNode = std::shared_ptr<STransformNode>;
            using GroupNode = std::shared_ptr<SGroupNode>;
            using ShapeNode = std::shared_ptr<SShapeNode>;
            
            struct SChunkHeader
            {
                char ID[4];
                int ChunkContentSize;
                int ChildChunkSize;
            };

            void LoadDefaultPalette();

            Math::Vec3i ProcessSize();
            void ProcessXYZI(VoxelModel m, const Math::Vec3i &_Size);
            std::vector<std::vector<SFrame>> ProcessMaterialAndSceneGraph();

            TransformNode ProcessTransformNode();
            GroupNode ProcessGroupNode();
            ShapeNode ProcessShapeNode();

            void SkipDict();

            std::map<int, int> m_ColorMapping;
            std::map<int, int> m_MaterialMapping;

            std::map<int, SceneNode> m_ModelSceneTreeMapping;

            std::vector<CColor> m_ColorPalette;

            size_t m_UsedColorsPos;
            bool m_HasEmission;
    };
}


#endif //VOXELFORMAT_HPP