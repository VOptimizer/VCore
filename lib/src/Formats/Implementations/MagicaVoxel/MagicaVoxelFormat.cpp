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

#include "MagicaVoxelFormat.hpp"
#include <string.h>
#include <sstream>
#include <stack>
#include <VCore/Misc/Exceptions.hpp>
#include "MagicaVoxelModelParser.hpp"
#include <VCore/Meshing/MaterialManager.hpp>
#include "MagicaVoxelStreamable.hpp"

namespace VCore
{
    struct SFrameSpeed
    {
        VoxelAnimation Anim;
        unsigned int FrameTime;
    };

    void CMagicaVoxelFormat::ClearCache()
    {
        IVoxelFormat::ClearCache();
        LoadDefaultPalette();

        m_UsedColorsPos = 0;
        m_ColorMapping.clear();
        m_MaterialMapping.clear();
        m_ModelSceneTreeMapping.clear();
        m_HasEmission = false;
        m_NotDefaultMaterials = nullptr;
    }

    void CMagicaVoxelFormat::LoadDefaultPalette()
    {
        for (size_t i = 0; i < PALETTE_SIZE; i++)
            memcpy(m_ColorPalette[i].c, &DefaultPalette[i], 4);
    }

    //////////////////////////////////////////////////
    // Writing functions
    //////////////////////////////////////////////////

    void CMagicaVoxelFormat::WriteFormat()
    {
        m_DataStream->Write("VOX ", 4);
        m_DataStream->Write((int32_t)150);

        SChunkHeader mainChunk = { {'M', 'A', 'I', 'N'}, 0, 0 };

        // Position to path later.
        auto patchPos = m_DataStream->Tell() + offsetof(SChunkHeader, ChildChunkSize);
        m_DataStream->Write(mainChunk);

        if(m_Models.size() > 1)
        {
            SChunkHeader packChunk = { {'P', 'A', 'C', 'K'}, (int)sizeof(int32_t), 0 };
            m_DataStream->Write(packChunk);
            m_DataStream->Write((int32_t)m_Models.size());
        }

        // Setup root of the scenetree
        // A scenetree starts always with a transform node followed by a group node.
        auto root = std::make_shared<STransformNode>();
        root->NodeID = 0;
        root->ChildID = 1;
        root->LayerID = -1;
        root->NumFrames = 1;
        m_MagicaSceneTree.push_back(root);

        auto rootGroup = std::make_shared<SGroupNode>();
        rootGroup->NodeID = 1;
        m_MagicaSceneTree.push_back(rootGroup);

        // Resets everything
        m_Materials.clear();
        LoadDefaultPalette();

        m_VoxelIndex = 1;
        m_ModelCounter = 0;

        // First try to traverse the scenetree
        TraverseVCoreSceneTree();

        // Write all Animations, if there are any.
        for (auto &&animation : m_Animations)
        {
            // Checks, if this animation was already written.
            if(m_AlreadyWrittenAnimations.empty() || (m_AlreadyWrittenAnimations.find(reinterpret_cast<uintptr_t>(animation.get())) == m_AlreadyWrittenAnimations.end()))
                rootGroup->ChildrensID.push_back(WriteAnimation(animation));
        }
        
        // Write all Models, if there are any.
        for (auto &&model : m_Models)
        {
            // Checks, if this model was already written.
            if(m_AlreadyWrittenModels.empty() || (m_AlreadyWrittenModels.find(reinterpret_cast<uintptr_t>(model.get())) == m_AlreadyWrittenModels.end()))
                rootGroup->ChildrensID.push_back(WriteModel(model));
        }

        m_AlreadyWrittenAnimations.clear();
        m_AlreadyWrittenModels.clear();

        // Write the scenetree to the file.
        WriteSceneTree();       

        // Write the used colors.
        SChunkHeader rgbaChunk = { {'R', 'G', 'B', 'A'}, (int)sizeof(int32_t) * 256, 0 };
        m_DataStream->Write(rgbaChunk);
        m_DataStream->Write((char*)m_ColorPalette, (int)sizeof(int32_t) * PALETTE_SIZE);

        // Write for each color a material.
        for (size_t i = 0; i < PALETTE_SIZE; i++)
        {
            auto matlPatchPos = m_DataStream->Tell() + offsetof(SChunkHeader, ChunkContentSize);
            SChunkHeader matlChunk = { {'M', 'A', 'T', 'L'}, 0, 0 };
            m_DataStream->Write(matlChunk);
            m_DataStream->Write((int32_t)i);

            Material mat;
            if(i > 0 && (i - 1) < m_Materials.size())
                mat = m_Materials[i - 1];
            else
                mat = MaterialManager::GetMaterial(0);

            m_DataStream->Write((int32_t)8);

            if(mat->Metallic)
                WriteDictKeyString("_type", "_metal");
            else if(mat->Transparency)
                WriteDictKeyString("_type", "_glass");
            else if(mat->Emission || mat->Power)
                WriteDictKeyString("_type", "_emit");
            else
                WriteDictKeyString("_type", "_diffuse");

            WriteDictKeyFloat("_metal", mat->Metallic);
            WriteDictKeyFloat("_alpha", mat->Transparency);
            WriteDictKeyFloat("_rough", mat->Roughness);
            WriteDictKeyFloat("_spec", mat->Specular);
            WriteDictKeyFloat("_ior", mat->IOR);
            WriteDictKeyFloat("_emit", mat->Emission);
            WriteDictKeyFloat("_flux", mat->Power);

            auto currentPos = m_DataStream->Tell();
            m_DataStream->Seek(matlPatchPos, SeekOrigin::BEG);
            m_DataStream->Write((int32_t)(currentPos - (matlPatchPos + 8)));
            m_DataStream->Seek(currentPos, SeekOrigin::BEG);
        }

        // Patch the main chunks child content size.
        auto currentPos = m_DataStream->Tell();
        m_DataStream->Seek(patchPos, SeekOrigin::BEG);
        m_DataStream->Write((int32_t)(currentPos - (patchPos + 4)));
        m_DataStream->Seek(currentPos, SeekOrigin::BEG);

        m_VoxelIndexMap.clear();
        m_Materials.clear();
        m_MagicaSceneTree.clear();
    }

    int CMagicaVoxelFormat::WriteAnimation(const VoxelAnimation &_Animation)
    {
        m_AlreadyWrittenAnimations.insert(reinterpret_cast<uintptr_t>(_Animation.get()));

        int result = 0;

        ankerl::unordered_dense::map<Math::Vec3i, ShapeNode, Math::Vec3iHasher> shapes;
        for (size_t i = 0; i < _Animation->GetFrameCount(); i++)
        {
            auto frame = _Animation->GetFrame(i);
            auto tmp = WriteModel(frame.Model, &shapes, (frame.FrameTime - CVoxelAnimation::FRAME_TIME) / CVoxelAnimation::FRAME_TIME);
            if(result == 0)
                result = tmp;
        }

        return result;
    }

    int CMagicaVoxelFormat::WriteModel(const VoxelModel &_Model, ankerl::unordered_dense::map<Math::Vec3i, ShapeNode, Math::Vec3iHasher> *_Shapes, uint32_t _FrameIdx)
    {
        m_AlreadyWrittenModels.insert(reinterpret_cast<uintptr_t>(_Model.get()));

        int scenetreeId = 0;
        bool generateNodes = (_Shapes == nullptr) || _Shapes->empty();

        auto bbox = _Model->calculateBBox();
        auto size = bbox.GetSize();

        if(generateNodes)
        {
            auto transform = std::make_shared<STransformNode>();
            transform->NodeID = m_MagicaSceneTree.size();
            transform->ChildID = m_MagicaSceneTree.size() + 1;
            transform->LayerID = 0;
            transform->NumFrames = 1;
            transform->Frames.push_back(SFrameTransform());
            transform->Frames[0].Translation = Math::Vec3f(bbox.Beg.x, bbox.Beg.y, bbox.Beg.z);
            if(!_Model->Name.empty())
                transform->Attributes["_name"] = _Model->Name;

            m_MagicaSceneTree.push_back(transform);
            scenetreeId = transform->NodeID;
        }

        GroupNode group;
        if(size.x >= 256 || size.y >= 256 || size.z >= 256)
        {
            group = std::make_shared<SGroupNode>();
            group->NodeID = m_MagicaSceneTree.size();
            m_MagicaSceneTree.push_back(group);
        }

        for (int32_t x = 0; x <= size.x; x += 256)
        {
            for (int32_t y = 0; y <= size.y; y += 256)
            {
                for (int32_t z = 0; z <= size.z; z += 256)
                {
                    if(generateNodes)
                    {
                        if(group)
                        {
                            auto transform = std::make_shared<STransformNode>();
                            transform->NodeID = m_MagicaSceneTree.size();
                            transform->ChildID = m_MagicaSceneTree.size() + 1;
                            transform->LayerID = 0;
                            transform->NumFrames = 1;
                            transform->Frames.push_back(SFrameTransform());
                            transform->Frames[0].Translation = Math::Vec3i(x, y, z);
                            group->ChildrensID.push_back(transform->NodeID);
                            m_MagicaSceneTree.push_back(transform);
                        }

                        auto shape = std::make_shared<SShapeNode>();
                        shape->NodeID = m_MagicaSceneTree.size();

                        SFrame frame;
                        frame.ModelId = m_ModelCounter++;
                        frame.FrameIdx = _FrameIdx;
                        shape->Models.push_back(frame);
                        m_MagicaSceneTree.push_back(shape);

                        if(_Shapes)
                            (*_Shapes)[Math::Vec3i(x, y, z)] = shape;
                    }
                    else
                    {
                        auto it = _Shapes->find(Math::Vec3i(x, y, z));
                        if(it == _Shapes->end())
                            continue;

                        SFrame frame;
                        frame.ModelId = m_ModelCounter++;
                        frame.FrameIdx = _FrameIdx;
                        it->second->Models.push_back(frame);
                        it->second->Attributes["_loop"] = "1";
                    }

                    SChunkHeader sizeChunk = { {'S', 'I', 'Z', 'E'}, (int)sizeof(int32_t) * 3, 0 };
                    m_DataStream->Write(sizeChunk);

            	    auto modelSize = size - Math::Vec3f(x, y, z);

                    m_DataStream->Write(std::min((int32_t)modelSize.x + 1, (int32_t)256));
                    m_DataStream->Write(std::min((int32_t)modelSize.z + 1, (int32_t)256));
                    m_DataStream->Write(std::min((int32_t)modelSize.y + 1, (int32_t)256));

                    Texture diffuse;
                    auto it = _Model->Textures.find(TextureType::DIFFIUSE);
                    if(it != _Model->Textures.end())
                        diffuse = it->second;

                    auto patchPos = m_DataStream->Tell() + offsetof(SChunkHeader, ChunkContentSize);
                    SChunkHeader xyziChunk = { {'X', 'Y', 'Z', 'I'}, 0, 0 };
                    m_DataStream->Write(xyziChunk);
                    // static_cast<int>(sizeof(int32_t) + sizeof(int32_t) * _Model->GetBlockCount())
                    m_DataStream->Write((int32_t)0);

                    ankerl::unordered_dense::map<Math::Vec3i, const IChunk*, Math::Vec3iHasher> chunks;
                    for (int mx = 0; mx <= modelSize.x; mx += Config::ChunkSize)
                    {
                        for (int my = 0; my <= modelSize.y; my += Config::ChunkSize)
                        {
                            for (int mz = 0; mz <= modelSize.z; mz += Config::ChunkSize)
                            {
                                auto chunkpos = GetChunkpos(Math::Vec3i(bbox.End.x - (mx + x), y + my, z + mz));
                                auto chunk = _Model->getChunk(chunkpos);
                                if(chunk)
                                    chunks[chunkpos] = chunk;
                            }
                        }
                    }

                    int32_t voxelCount = 0;
                    for (auto &&chunk : chunks)
                    {
                        auto innerBBox = chunk.second->inner_bbox(Math::Vec3i());
                        for (int mx = innerBBox.End.x; mx >= innerBBox.Beg.x; mx--)
                        {
                            for (int my = innerBBox.Beg.y; my <= innerBBox.End.y; my++)
                            {
                                for (int mz = innerBBox.Beg.z; mz <= innerBBox.End.z; mz++)
                                {
                                    auto relpos = chunk.first - (chunk.first & 256);

                                    // Mirrors the model for MagicaVoxel
                                    auto voxel = chunk.second->find(Math::Vec3i(mx, my, mz));
                                    if(voxel.IsInstantiated())
                                    {
                                        voxelCount++;
                                        m_DataStream->Write((uint8_t)(modelSize.x - ((mx - innerBBox.Beg.x) + relpos.x)));
                                        m_DataStream->Write((uint8_t)((mz - innerBBox.Beg.z) + relpos.z));
                                        m_DataStream->Write((uint8_t)((my - innerBBox.Beg.y) + relpos.y));

                                        auto mapIt = m_VoxelIndexMap.find((uint32_t)voxel);
                                        if(mapIt == m_VoxelIndexMap.end())
                                        {
                                            if(diffuse)
                                                m_ColorPalette[m_VoxelIndex - 1] = diffuse->GetPixel(Math::Vec2ui(voxel.Color, 0));

                                            auto mat = MaterialManager::GetMaterial(voxel.Material);
                                            if(mat)
                                                m_Materials.push_back(mat);
                                            else
                                                m_Materials.push_back(MaterialManager::GetMaterial(0));

                                            mapIt = m_VoxelIndexMap.insert({(uint32_t)voxel, m_VoxelIndex++}).first;
                                        }

                                        m_DataStream->Write(mapIt->second);
                                    }
                                }
                            }
                        }
                    }                                       

                    auto currentPos = m_DataStream->Tell();
                    m_DataStream->Seek(patchPos, SeekOrigin::BEG);
                    m_DataStream->Write((int32_t)(currentPos - (patchPos + 8)));
                    m_DataStream->Seek(4, SeekOrigin::CUR);
                    m_DataStream->Write((int32_t)voxelCount);
                    m_DataStream->Seek(currentPos, SeekOrigin::BEG);
                }
            }
        }

        return scenetreeId;
    }

    void CMagicaVoxelFormat::WriteString(const char *_String)
    {
        uint32_t size = strlen(_String);

        // Size "prefix"
        m_DataStream->Write(size);

        // String without null termination.
        m_DataStream->Write(_String, size);
    }

    void CMagicaVoxelFormat::WriteDictKeyString(const char *_Key, const char *_Value)
    {
        WriteString(_Key);
        WriteString(_Value);
    }

    void CMagicaVoxelFormat::WriteDictKeyInt(const char *_Key, const int _Value)
    {
        WriteString(_Key);

        // Formats the float number to a string with two digits after the decimal point.
        uint32_t size = snprintf(nullptr, 0, "%i", _Value);
        char *value = new char[size + 1];
        snprintf(value, size + 1, "%i", _Value);
        
        m_DataStream->Write(size);
        m_DataStream->Write(value, size);
        delete[] value;
    }

    void CMagicaVoxelFormat::WriteDictKeyFloat(const char *_Key, const float _Value)
    {
        WriteString(_Key);

        // Formats the float number to a string with two digits after the decimal point.
        uint32_t size = snprintf(nullptr, 0, "%.2f", _Value);
        char *value = new char[size + 1];
        snprintf(value, size + 1, "%.2f", _Value);
        
        m_DataStream->Write(size);
        m_DataStream->Write(value, size);
        delete[] value;
    }

    void CMagicaVoxelFormat::WriteDictKeyVec3i(const char *_Key, const Math::Vec3i &_Value)
    {
        WriteString(_Key);

        // Formats a Vec3i in the format "X Z Y", since MagicaVoxel uses the Z Axis as gravity axis.
        uint32_t size = snprintf(nullptr, 0, "%i %i %i", _Value.x, _Value.z, _Value.y);
        char *value = new char[size + 1];
        snprintf(value, size + 1, "%i %i %i", _Value.x, _Value.z, _Value.y);
        
        m_DataStream->Write(size);
        m_DataStream->Write(value, size);
        delete[] value;
    }

    void CMagicaVoxelFormat::TraverseVCoreSceneTree()
    {
        auto rootGroup = std::static_pointer_cast<SGroupNode>(m_MagicaSceneTree[m_MagicaSceneTree.size() - 1]);
        if(m_SceneTree)
            rootGroup->ChildrensID.push_back(TraverseSceneTreeNode(m_SceneTree));
    }

    int CMagicaVoxelFormat::TraverseSceneTreeNode(const SceneNode &_Node)
    {
        if(_Node->Model)
        {
            auto childId = WriteModel(_Node->Model);
            auto translate = std::static_pointer_cast<STransformNode>(m_MagicaSceneTree[childId]);
            translate->Frames[0].Translation += _Node->Position + _Node->Model->calculateBBox().GetSize() * 0.5f;
            if(!_Node->Visible)
                translate->Attributes["_hidden"] = "1";
            return childId;
        }
        else if(_Node->Animation)
        {
            auto childId = WriteAnimation(_Node->Animation);
            auto translate = std::static_pointer_cast<STransformNode>(m_MagicaSceneTree[childId]);
            translate->Frames[0].Translation += _Node->Position + _Node->Animation->GetFrame(0).Model->calculateBBox().GetSize() * 0.5f;
            if(!_Node->Visible)
                translate->Attributes["_hidden"] = "1";
            return childId;
        }

        auto transform = std::make_shared<STransformNode>();
        transform->NodeID = m_MagicaSceneTree.size();
        transform->ChildID = m_MagicaSceneTree.size() + 1;
        transform->LayerID = 0;
        transform->NumFrames = 1;
        transform->Frames.push_back(SFrameTransform());
        transform->Frames[0].Translation = _Node->Position; //Math::Vec3i(_Node->Position.x, _Node->Position.z, _Node->Position.y);
        // transform->Frames[0].Rotation = Math::Vec3i(_Node->Rotation.x, _Node->Rotation.z, _Node->Rotation.y);
        m_MagicaSceneTree.push_back(transform);

        auto group = std::make_shared<SGroupNode>();
        group->NodeID = m_MagicaSceneTree.size();
        m_MagicaSceneTree.push_back(group);

        for (auto &&node : *_Node)
            group->ChildrensID.push_back(TraverseSceneTreeNode(node));

        return transform->NodeID;
    }

    void CMagicaVoxelFormat::WriteSceneTree()
    {
        for (auto &&node : m_MagicaSceneTree)
        {
            // Position to path later.
            auto patchPos = m_DataStream->Tell() + offsetof(SChunkHeader, ChunkContentSize);

            switch (node->Type)
            {
                case NodeType::TRANSFORM: {
                    auto transform = std::static_pointer_cast<STransformNode>(node);

                    SChunkHeader nTRNChunk = { {'n', 'T', 'R', 'N'}, 0, 0 };
                    m_DataStream->Write(nTRNChunk);
                    m_DataStream->Write(transform->NodeID);
                    m_DataStream->Write((int32_t)transform->Attributes.size());
                    for (auto &&dict : transform->Attributes)
                        WriteDictKeyString(dict.first.c_str(), dict.second.c_str());
                    m_DataStream->Write(transform->ChildID);
                    m_DataStream->Write((int32_t)-1);
                    m_DataStream->Write(transform->LayerID);
                    m_DataStream->Write(transform->NumFrames);

                    for (auto &&frame : transform->Frames)
                    {
                        m_DataStream->Write((int32_t)1);
                        WriteDictKeyVec3i("_t", frame.Translation);
                        // WriteDictKeyInt("_f", frame.FrameIdx);
                    }

                    if(transform->Frames.size() == 0)
                        m_DataStream->Write((int32_t)0);
                } break;
            
                case NodeType::GROUP: {
                    auto group = std::static_pointer_cast<SGroupNode>(node);

                    SChunkHeader nGRPChunk = { {'n', 'G', 'R', 'P'}, 0, 0 };
                    m_DataStream->Write(nGRPChunk);
                    m_DataStream->Write(group->NodeID);
                    m_DataStream->Write((int32_t)0);
                    m_DataStream->Write((int32_t)group->ChildrensID.size());

                    for (auto &&child : group->ChildrensID)
                        m_DataStream->Write((int32_t)child);
                } break;

                case NodeType::SHAPE: {
                    auto shape = std::static_pointer_cast<SShapeNode>(node);

                    SChunkHeader nSHPChunk = { {'n', 'S', 'H', 'P'}, 0, 0 };
                    m_DataStream->Write(nSHPChunk);
                    m_DataStream->Write(shape->NodeID);
                    // m_DataStream->Write((int32_t)0);

                    m_DataStream->Write((int32_t)shape->Attributes.size());
                    for (auto &&dict : shape->Attributes)
                        WriteDictKeyString(dict.first.c_str(), dict.second.c_str());

                    m_DataStream->Write((int32_t)shape->Models.size());

                    for (auto &&model : shape->Models)
                    {
                        m_DataStream->Write((int32_t)model.ModelId);

                        if(shape->Models.size() == 1)
                            m_DataStream->Write((int32_t)0);
                        else
                        {
                            m_DataStream->Write((int32_t)1);
                            WriteDictKeyInt("_f", model.FrameIdx);
                        }
                    }
                } break;
            }

            auto currentPos = m_DataStream->Tell();
            m_DataStream->Seek(patchPos, SeekOrigin::BEG);
            m_DataStream->Write((int32_t)(currentPos - (patchPos + 8)));
            m_DataStream->Seek(currentPos, SeekOrigin::BEG);
        }
    }


    //////////////////////////////////////////////////
    // Reading functions
    //////////////////////////////////////////////////

    void CMagicaVoxelFormat::ParseFormat()
    {   
        m_ColorpaletterPosition = UINT64_MAX;

        std::string Signature(4, '\0');
        m_DataStream->Read(&Signature[0], 4);
        Signature += "\0";

        // Checks the file header
        if(Signature != "VOX ")
            throw CVoxelLoaderException("Unknown file format");

        int Version = m_DataStream->Read<int>();
        if(Version < 150)
            throw CVoxelLoaderException("Version: " + std::to_string(Version) + " is not supported");

        ankerl::unordered_dense::map<int, SFrameSpeed> animations;

        // First processes the materials that are at the end of the file. 
        auto anims = ProcessMaterialAndSceneGraph();
        // TODO: This is very ugly, but I'm currently clueless.
        for (auto &&frames : anims)
        {
            VoxelAnimation anim = std::make_shared<CVoxelAnimation>();
            m_Animations.push_back(anim);

            for (auto &&frame : frames)
            {
                SFrameSpeed speed;
                speed.Anim = anim;
                speed.FrameTime = frame.FrameIdx * CVoxelAnimation::FRAME_TIME + CVoxelAnimation::FRAME_TIME;
                animations[frame.ModelId] = speed;
            }
        }
        

        m_DataStream->Seek(8);

        if(!m_DataStream->Eof())
        {
            SChunkHeader Tmp = m_DataStream->Read<SChunkHeader>();
            if(strncmp(Tmp.ID, "MAIN", sizeof(Tmp.ID)) == 0)
            {
                while (!m_DataStream->Eof())
                {
                    Tmp = m_DataStream->Read<SChunkHeader>();

                    if(strncmp(Tmp.ID, "SIZE", sizeof(Tmp.ID)) == 0)
                    {
                        VoxelModel m;
                        Math::Vec3i size;
                        if(static_cast<unsigned>(m_Mode) & static_cast<unsigned>(FileMode::STREAMED))
                        {
                            m = std::make_shared<CVoxelSpace>(new CMagicaVoxelStreamable(m_IOHandler, m_ColorpaletterPosition, m_DataStream->Tell(), m_NotDefaultMaterials, m_DataStream->GetFilePath()));
                            size = CMagicaVoxelModelParser::ProcessSize(m_DataStream);
                        }
                        else
                        {
                            m = std::make_shared<CVoxelSpace>();
                            CMagicaVoxelModelParser parser(m_DataStream, m_ColorpaletterPosition, m_DataStream->Tell(), m_NotDefaultMaterials);
                            parser.FillVoxelSpace(*m);
                            size = parser.GetSize();
                        }

                        m_Models.push_back(m);

                        Math::Vec3i halfSize = (size * 0.5);
                        auto treeNode = m_ModelSceneTreeMapping.at(m_Models.size() - 1);

                        bool isAnimation = false;
                        auto frame = animations.find(m_Models.size() - 1);
                        if(frame != animations.end())
                        {
                            isAnimation = true;
                            frame->second.Anim->AddFrame(m, frame->second.FrameTime);
                        }

                        if(isAnimation && !treeNode->Animation)
                        {    
                            auto pos = treeNode->Position;

                            // Since we are in voxelspace, which begins at 0, 0, 0 and ends at max. 255, 255, 255
                            // it's necessary to substract the center of this space from the global world space
                            // in order to get the correct result
                            treeNode->Position = pos - halfSize;

                            treeNode->Animation = frame->second.Anim; 
                            m->Name = treeNode->Name;  
                        }
                        else if(!isAnimation && !treeNode->Model)
                        {
                            auto pos = treeNode->Position;

                            // Since we are in voxelspace, which begins at 0, 0, 0 and ends at max. 255, 255, 255
                            // it's necessary to substract the center of this space from the global world space
                            // in order to get the correct result
                            treeNode->Position = pos - halfSize;

                            treeNode->Model = m; 
                            m->Name = treeNode->Name;  
                        }                   
                    }
                    else
                        m_DataStream->Seek(Tmp.ChunkContentSize + Tmp.ChildChunkSize);
                }
            }
        }

        ankerl::unordered_dense::map<TextureType, Texture> textures;
        textures[TextureType::DIFFIUSE] = std::make_shared<CTexture>(Math::Vec2ui(m_ColorMapping.size(), 1));

        if(m_HasEmission)
            textures[TextureType::EMISSION] = std::make_shared<CTexture>(Math::Vec2ui(m_ColorMapping.size(), 1));

        // Creates the used color palette.
        for (auto &&c : m_ColorMapping)
        {
            textures[TextureType::DIFFIUSE]->AddPixel(m_ColorPalette[c.first - 1], Math::Vec2ui(c.second, 0));
            if(m_HasEmission)
            {
                int MatIdx = 0;
                auto IT = m_MaterialMapping.find(c.first);
                if(IT != m_MaterialMapping.end())
                    MatIdx = IT->second;

                auto material = m_Materials[MatIdx];
                if(material->Power > 0)
                    textures[TextureType::EMISSION]->AddPixel(m_ColorPalette[c.first - 1], Math::Vec2ui(c.second, 0));
            }
        }

        for (auto &&m : m_Models)
            m->Textures = textures;
    }

    fast_vector<fast_vector<SFrame>> CMagicaVoxelFormat::ProcessMaterialAndSceneGraph()
    {
        ankerl::unordered_dense::map<int, Node> nodes;
        fast_vector<fast_vector<SFrame>> ret;
        m_Materials.push_back(MaterialManager::GetMaterial(0));

        if(!m_DataStream->Eof())
        {
            SChunkHeader Tmp = m_DataStream->Read<SChunkHeader>();
            if(strncmp(Tmp.ID, "MAIN", sizeof(Tmp.ID)) == 0)
            {
                while (!m_DataStream->Eof())
                {
                    Tmp = m_DataStream->Read<SChunkHeader>();

                    if(strncmp(Tmp.ID, "MATL", sizeof(Tmp.ID)) == 0)
                    {
                        CMaterial material;
                        auto materialIdx = m_DataStream->Read<uint32_t>();
                        auto keyValueCount = m_DataStream->Read<uint32_t>();

                        for (uint32_t i = 0; i < keyValueCount; i++)
                        {
                            // Reads the key.
                            auto strLen = m_DataStream->Read<uint32_t>();
                            std::string key(strLen, '\0'); 
                            m_DataStream->Read(&key[0], strLen);

                            if(key == "_plastic")
                                continue;

                            // Reads the value.
                            strLen = m_DataStream->Read<uint32_t>();
                            std::string value(strLen, '\0'); 
                            m_DataStream->Read(&value[0], strLen);

                            if(key == "_metal")
                                material.Metallic = std::stof(value);
                            else if(key == "_alpha")
                                material.Transparency = std::stof(value);     
                            else if(key == "_rough")
                                material.Roughness = std::stof(value);
                            else if(key == "_spec")
                                material.Specular = std::stof(value);
                            else if(key == "_ior")
                                material.IOR = std::stof(value);
                            else if(key == "_flux")
                                material.Power = std::stof(value);
                            else if(key == "_emit")
                                material.Emission = std::stof(value);
                        }

                        auto defaultMaterial = MaterialManager::GetMaterial(0);
                        if(*defaultMaterial != material)
                        {
                            if(!m_NotDefaultMaterials)
                                m_NotDefaultMaterials = std::make_shared<ankerl::unordered_dense::map<uint8_t, CMaterial>>();

                            m_NotDefaultMaterials->insert({materialIdx, material});
                        }
                    }
                    else if(strncmp(Tmp.ID, "RGBA", sizeof(Tmp.ID)) == 0)
                    {
                        m_ColorpaletterPosition = m_DataStream->Tell() - sizeof(SChunkHeader);
                        m_DataStream->Seek(Tmp.ChunkContentSize + Tmp.ChildChunkSize);
                    }
                    else if(strncmp(Tmp.ID, "nTRN", sizeof(Tmp.ID)) == 0)
                    {
                        auto tmp = ProcessTransformNode();
                        nodes.insert({tmp->NodeID, tmp});
                    }
                    else if(strncmp(Tmp.ID, "nGRP", sizeof(Tmp.ID)) == 0)
                    {
                        auto tmp = ProcessGroupNode();
                        nodes.insert({tmp->NodeID, tmp});
                    }
                    else if(strncmp(Tmp.ID, "nSHP", sizeof(Tmp.ID)) == 0)
                    {
                        auto tmp = ProcessShapeNode();
                        nodes.insert({tmp->NodeID, tmp});

                        if(tmp->Models.size() > 1)
                            ret.push_back(tmp->Models);
                    }
                    else
                        m_DataStream->Seek(Tmp.ChunkContentSize + Tmp.ChildChunkSize);
                }
            }
        }

        if(!nodes.empty()) 
        {
            std::stack<int> nodeIDs;
            std::stack<SceneNode> sceneNodes;

            SceneNode currentNode = m_SceneTree;

            nodeIDs.push(0);
            while (!nodeIDs.empty())
            {
                Node tmp = nodes[nodeIDs.top()];

                switch (tmp->Type)
                {
                    case NodeType::TRANSFORM:
                    {
                        auto transform = std::static_pointer_cast<STransformNode>(tmp);
                        nodeIDs.pop();

                        currentNode->Position = transform->Frames[0].Translation;
                        currentNode->Rotation = transform->Frames[0].Rotation;
                        currentNode->Name = transform->Name;

                        nodeIDs.push(transform->ChildID);
                    } break;

                    case NodeType::GROUP:
                    {
                        auto group = std::static_pointer_cast<SGroupNode>(tmp);
                        if(group->ChildIdx > 0)
                        {
                            currentNode = sceneNodes.top();
                            sceneNodes.pop();
                        }

                        if((size_t)group->ChildIdx < group->ChildrensID.size())
                        {
                            sceneNodes.push(currentNode);
                            auto oldCurrent = currentNode;
                            currentNode = std::make_shared<CSceneNode>();
                            oldCurrent->AddChild(currentNode);

                            nodeIDs.push(group->ChildrensID[group->ChildIdx]);
                            group->ChildIdx++;
                        }
                        else                     
                            nodeIDs.pop();
                    } break;

                    case NodeType::SHAPE:
                    {
                        nodeIDs.pop();
                        auto shapes = std::static_pointer_cast<SShapeNode>(tmp);

                        // if(!shapes->Models.empty())
                        //     m_ModelSceneTreeMapping.insert({shapes->Models[0].ModelId, currentNode});

                        for (auto &&m : shapes->Models)
                        {
                            m_ModelSceneTreeMapping.insert({m.ModelId, currentNode});
                        }             
                    } break;
                }
            }
        }
        else
            m_ModelSceneTreeMapping.insert({0, m_SceneTree});

        m_DataStream->Seek(0, SeekOrigin::BEG);

        return ret;
    }

    TransformNode CMagicaVoxelFormat::ProcessTransformNode()
    {
        TransformNode Ret = TransformNode(new STransformNode());

        Ret->NodeID = m_DataStream->Read<int>();
        
        // Skips the dictionary
        int keys = m_DataStream->Read<int>();
        for (int i = 0; i < keys; i++)
        {
            int size = m_DataStream->Read<int>();
            std::string key(size, '\0'); 
            m_DataStream->Read(&key[0], size);
            if(key == "_name")
            {              
                size = m_DataStream->Read<int>();
                std::string value(size, '\0'); 
                m_DataStream->Read(&value[0], size);
                Ret->Name = value;
            }
            else
            {
                int size = m_DataStream->Read<int>();
                m_DataStream->Seek(size);
            }            
        }

        Ret->ChildID = m_DataStream->Read<int>();
        m_DataStream->Seek(sizeof(int));
        Ret->LayerID = m_DataStream->Read<int>();

        int frames = m_DataStream->Read<int>();
        for (int i = 0; i < frames; i++)
        {
            SFrameTransform frameTransform;

            keys = m_DataStream->Read<int>();
            for (int j = 0; j < keys; j++)
            {
                int size = m_DataStream->Read<int>();
                std::string Key(size, '\0'); 
                m_DataStream->Read(&Key[0], size);

                if(Key == "_t")
                {
                    size = m_DataStream->Read<int>();
                    std::string Value(size, '\0'); 
                    m_DataStream->Read(&Value[0], size);

                    std::stringstream tmp;
                    tmp << Value;

                    // Scenetree always in OpenGL Y-UP Space
                    tmp >> frameTransform.Translation.x >> frameTransform.Translation.z >> frameTransform.Translation.y;
                    frameTransform.Translation.x *= -1;
                }
                else if(Key == "_r")
                {
                    size = m_DataStream->Read<int>();
                    std::string Value(size, '\0'); 
                    m_DataStream->Read(&Value[0], size);

                    char rot = std::stoi(Value);

                    uint8_t idx1 = rot & 3;
                    uint8_t idx2 = (rot >> 2) & 3;
                    uint8_t idx3 = 3 - idx1 - idx2;

                    auto rotation = Math::Mat4x4(Math::Vec4f(0, 0, 0, 0),
                                            Math::Vec4f(0, 0, 0, 0),
                                            Math::Vec4f(0, 0, 0, 0),
                                            Math::Vec4f(0, 0, 0, 1));

                    rotation.x.v[idx1] = ((rot & 0x10) == 0x10) ? -1 : 1;
                    rotation.y.v[idx2] = ((rot & 0x20) == 0x20) ? -1 : 1;
                    rotation.z.v[idx3] = ((rot & 0x40) == 0x40) ? -1 : 1;

                    // Gets the euler angle, y is the up axis.
                    frameTransform.Rotation = rotation.GetEuler();
                    std::swap(frameTransform.Rotation.y, frameTransform.Rotation.z);
                }
                else if(Key == "_f")
                {
                    int size = m_DataStream->Read<int>();
                    std::string value(size, '\0'); 
                    m_DataStream->Read(&value[0], size);

                    frameTransform.FrameIdx = std::stoi(value);
                }
            }

            Ret->Frames.push_back(frameTransform);
        }

        return Ret;
    }
    
    GroupNode CMagicaVoxelFormat::ProcessGroupNode()
    {
        GroupNode Ret = GroupNode(new SGroupNode());

        Ret->NodeID = m_DataStream->Read<int>();
        SkipDict();

        int childs = m_DataStream->Read<int>();
        for (int i = 0; i < childs; i++)
            Ret->ChildrensID.push_back(m_DataStream->Read<int>());

        return Ret;
    }

    ShapeNode CMagicaVoxelFormat::ProcessShapeNode()
    {
        ShapeNode Ret = ShapeNode(new SShapeNode());

        Ret->NodeID = m_DataStream->Read<int>();
        SkipDict();

        int childs = m_DataStream->Read<int>();
        for (int i = 0; i < childs; i++)
        {
            SFrame frame;
            frame.ModelId = m_DataStream->Read<int>();

            // Skips the dictionary
            int keys = m_DataStream->Read<int>();
            for (int i = 0; i < keys; i++)
            {
                int size = m_DataStream->Read<int>();
                std::string key(size, '\0'); 
                m_DataStream->Read(&key[0], size);

                if(key == "_f")
                {
                    int size = m_DataStream->Read<int>();
                    std::string value(size, '\0'); 
                    m_DataStream->Read(&value[0], size);

                    frame.FrameIdx = std::stoi(value);
                }
                else
                    m_DataStream->Seek(m_DataStream->Read<int>());
            }

            Ret->Models.push_back(frame);
        }

        return Ret;
    }

    void CMagicaVoxelFormat::SkipDict()
    {
        int keys = m_DataStream->Read<int>();
        for (int i = 0; i < keys; i++)
        {
            int size = m_DataStream->Read<int>();
            m_DataStream->Seek(size);
            size = m_DataStream->Read<int>();
            m_DataStream->Seek(size);
        }
    }
}
