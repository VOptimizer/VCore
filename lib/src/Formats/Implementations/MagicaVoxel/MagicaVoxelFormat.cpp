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
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <VCore/Misc/Exceptions.hpp>
#include "MagicaVoxelDictionary.hpp"
#include "MagicaVoxelModelParser.hpp"
#include <VCore/Meshing/MaterialManager.hpp>
#include "MagicaVoxelScenetreeWriter.hpp"
#include "MagicaVoxelStreamable.hpp"
#include <VCore/Formats/SceneNode.hpp>
#include <VCore/Math/Mat4x4.hpp>
#include <VCore/Math/Vector.hpp>
#include <VCore/Misc/FileStream.hpp>
#include <VCore/Misc/fast_vector.hpp>
#include <VCore/Debug.hpp>
#include <string>
#include <string_view>
#include <cstdio>

namespace VCore
{
    constexpr auto MAIN_CHUNK_ID = MakeChunkId('M', 'A', 'I', 'N');
    constexpr auto MATL_CHUNK_ID = MakeChunkId('M', 'A', 'T', 'L');
    constexpr auto RGBA_CHUNK_ID = MakeChunkId('R', 'G', 'B', 'A');
    constexpr auto TRANSFORM_CHUNK_ID = MakeChunkId('n', 'T', 'R', 'N');
    constexpr auto GROUP_CHUNK_ID = MakeChunkId('n', 'G', 'R', 'P');
    constexpr auto SHAPE_CHUNK_ID = MakeChunkId('n', 'S', 'H', 'P');
    constexpr auto SIZE_CHUNK_ID = MakeChunkId('S', 'I', 'Z', 'E');
    constexpr auto PACK_CHUNK_ID = MakeChunkId('P', 'A', 'C', 'K');
    constexpr auto XYZI_CHUNK_ID = MakeChunkId('X', 'Y', 'Z', 'I');

    struct SMagicaVoxelSceneTreeHelper
    {
        ankerl::unordered_dense::map<uint32_t, CSceneNodeBase*> Groups;
        ankerl::unordered_dense::map<uint32_t, CSceneNodeBase*> Orphans;
        ankerl::unordered_dense::map<uint32_t, STransformNode> ParentTransforms;

        /** Build the tree further with each iteration. */
        void BuildTree(uint32_t p_NodeId, CSceneNodeBase *p_Node)
        {
            uint32_t parentId = static_cast<uint32_t>(-1);

            // Sets the transform and name of the current group node.
            auto it = ParentTransforms.find(p_NodeId);
            if(it != ParentTransforms.end()) [[likely]]
            {
                p_Node->Name = it->second.Name;
                p_Node->Visible = !it->second.Hidden;

                if(!it->second.Frames.empty())
                {
                    p_Node->SetPosition(it->second.Frames[0].Translation);
                    p_Node->SetRotation(it->second.Frames[0].Rotation);
                    p_Node->SetScale(it->second.Frames[0].Scale);
                }

                parentId = it->second.NodeId;
                ParentTransforms.erase(it);
            }

            auto groupIt = Groups.find(parentId);
            if(groupIt != Groups.end())
            {
                groupIt->second->AddChild(p_Node);
                Groups.erase(groupIt);
            }
            else
                Orphans[p_NodeId] = p_Node;
        }

        void BuildTree(CSceneNodeBase *p_Node, const fast_vector<uint32_t> &p_Children)
        {
            for (auto &&child: p_Children) 
            {
                auto modelIt = Orphans.find(child);
                if(modelIt != Orphans.end())
                {
                    p_Node->AddChild(modelIt->second);
                    Orphans.erase(modelIt);
                }
                else
                    Groups[child] = p_Node;
            }
        }
    };

    int ConvertRangeToInt(std::string_view::const_iterator p_Begin, std::string_view::const_iterator p_End)
    {
        int result = 0;
        bool negative = false;
        while (p_Begin != p_End) 
        {
            if(*p_Begin == '-')
                negative = true;
            else
                result = result * 10 + (*p_Begin - '0');

            p_Begin++;
        }

        return result * (negative ? -1 : 1);
    }

    Math::Vec3i ParsePosition(std::string_view p_View)
    {
        Math::Vec3i result;
        auto spaceIt = std::find(p_View.begin(), p_View.end(), ' ');

        result.x = ConvertRangeToInt(p_View.begin(), spaceIt);

        // Scenetree always in OpenGL Y-UP Space
        auto begin = spaceIt + 1;
        spaceIt = std::find(begin, p_View.end(), ' ');
        result.z = ConvertRangeToInt(begin, spaceIt);

        begin = spaceIt + 1;
        spaceIt = std::find(begin, p_View.end(), ' ');
        result.y = ConvertRangeToInt(begin, spaceIt);

        return result;
    }

    void CMagicaVoxelFormat::ClearCache()
    {
        IVoxelFormat::ClearCache();
        LoadDefaultPalette();

        m_MaterialMap = nullptr;
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
        if(!SceneTree)
            return;

        m_DataStream->Write("VOX ", 4);
        m_DataStream->Write((int32_t)150);

        SMagicaVoxelChunkHeader mainChunk = { MAIN_CHUNK_ID, 0, 0 };

        // Position to path later.
        auto patchPos = m_DataStream->Tell() + offsetof(SMagicaVoxelChunkHeader, ChildChunkSize);
        m_DataStream->Write(mainChunk);

        auto modelCount = SceneTree->GetModels().size();
        if(modelCount > 1)
        {
            SMagicaVoxelChunkHeader packChunk = { PACK_CHUNK_ID, (int)sizeof(int32_t), 0 };
            m_DataStream->Write(packChunk);
            m_DataStream->Write((int32_t)modelCount);
        }

        // Setup root of the scenetree
        // A scenetree starts always with a transform node followed by a group node.
        // auto root = std::make_shared<STransformNode>();
        // root->NodeID = 0;
        // root->ChildID = 1;
        // root->LayerID = -1;
        // root->NumFrames = 1;
        // m_MagicaSceneTree.push_back(root);

        // auto rootGroup = std::make_shared<SGroupNode>();
        // rootGroup->NodeID = 1;
        // m_MagicaSceneTree.push_back(rootGroup);

        // Resets everything
        LoadDefaultPalette();

        m_VoxelIndex = 1;
        m_ModelCounter = 0;

        // First try to traverse the scenetree
        // TraverseVCoreSceneTree();

        // Write all Animations, if there are any.
        // for (auto &&animation : m_Animations)
        // {
        //     // Checks, if this animation was already written.
        //     if(m_AlreadyWrittenAnimations.empty() || (m_AlreadyWrittenAnimations.find(reinterpret_cast<uintptr_t>(animation.get())) == m_AlreadyWrittenAnimations.end()))
        //         rootGroup->ChildrensID.push_back(WriteAnimation(animation));
        // }

        auto modelCounter = 0;
        CMagicaVoxelScenetreeWriter tree(SceneTree, m_DataStream);
        
        // Write all Models, if there are any.
        for (auto &&model : SceneTree->GetModels())
        {
            tree.VoxelModelMap[modelCounter] = {};
            WriteModel(model, &tree, modelCounter);
            modelCounter++;
        }
            
        // {
        //     // Checks, if this model was already written.
        //     // if(m_AlreadyWrittenModels.empty() || (m_AlreadyWrittenModels.find(reinterpret_cast<uintptr_t>(model.get())) == m_AlreadyWrittenModels.end()))
        //     //     rootGroup->ChildrensID.push_back(WriteModel(model));
        // }

        // Write the scenetree to the file.
        tree.WriteTree();
        // WriteSceneTree();       

        // Write the used colors.
        SMagicaVoxelChunkHeader rgbaChunk = { RGBA_CHUNK_ID, (int)sizeof(int32_t) * 256, 0 };
        m_DataStream->Write(rgbaChunk);
        m_DataStream->Write((char*)m_ColorPalette, (int)sizeof(int32_t) * PALETTE_SIZE);


        CMagicaVoxelDictionary dict(m_DataStream);

        // Write for each color a material.
        for (size_t i = 0; i < PALETTE_SIZE; i++)
        {
            auto matlPatchPos = m_DataStream->Tell() + offsetof(SMagicaVoxelChunkHeader, ChunkContentSize);
            SMagicaVoxelChunkHeader matlChunk = { MATL_CHUNK_ID, 0, 0 };
            m_DataStream->Write(matlChunk);
            m_DataStream->Write((int32_t)i);

            Material mat = nullptr;
            if(i > 0 && (i - 1) < m_Materials.size())
                mat = MaterialManager::GetMaterial(m_Materials[i - 1]);
            
            if(!mat)
                mat = MaterialManager::GetMaterial(0);

            m_DataStream->Write((int32_t)8);

            if(mat->Metallic)
                dict.WriteDictKeyString("_type", "_metal");
            else if(mat->Transparency)
                dict.WriteDictKeyString("_type", "_glass");
            else if(mat->Emission || mat->Power)
                dict.WriteDictKeyString("_type", "_emit");
            else
                dict.WriteDictKeyString("_type", "_diffuse");

            dict.WriteDictKeyFloat("_metal", mat->Metallic);
            dict.WriteDictKeyFloat("_alpha", mat->Transparency);
            dict.WriteDictKeyFloat("_rough", mat->Roughness);
            dict.WriteDictKeyFloat("_spec", mat->Specular);
            dict.WriteDictKeyFloat("_ior", mat->IOR);
            dict.WriteDictKeyFloat("_emit", mat->Emission);
            dict.WriteDictKeyFloat("_flux", mat->Power);

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

    // int CMagicaVoxelFormat::WriteAnimation(const VoxelAnimation &p_Animation)
    // {
    //     m_AlreadyWrittenAnimations.insert(reinterpret_cast<uintptr_t>(p_Animation.get()));

    //     int result = 0;

    //     ankerl::unordered_dense::map<Math::Vec3i, SShapeNode, Math::Vec3iHasher> shapes;
    //     for (size_t i = 0; i < p_Animation->GetFrameCount(); i++)
    //     {
    //         auto frame = p_Animation->GetFrame(i);
    //         auto tmp = WriteModel(frame.Model, &shapes, (frame.FrameTime - CVoxelAnimation::FRAME_TIME) / CVoxelAnimation::FRAME_TIME);
    //         if(result == 0)
    //             result = tmp;
    //     }

    //     return result;
    // }

    void CMagicaVoxelFormat::WriteModel(const VoxelModel &p_Model, CMagicaVoxelScenetreeWriter *p_Tree, uint32_t p_modelCounter)
    {
        auto bbox = p_Model->CalculateBBox();
        auto size = bbox.GetSize();

        for (int32_t x = 0; x <= size.x; x += 256)
        {
            for (int32_t y = 0; y <= size.y; y += 256)
            {
                for (int32_t z = 0; z <= size.z; z += 256)
                {
                    p_Tree->VoxelModelMap[p_modelCounter].push_back(m_ModelCounter++);

                    SMagicaVoxelChunkHeader sizeChunk = { SIZE_CHUNK_ID, (int)sizeof(int32_t) * 3, 0 };
                    m_DataStream->Write(sizeChunk);

            	    auto modelSize = (size - Math::Vec3f(x, y, z) + Math::Vec3f::ONE).min(Math::Vec3f(256, 256, 256));
                    m_DataStream->Write((int32_t)modelSize.x);
                    m_DataStream->Write((int32_t)modelSize.z);
                    m_DataStream->Write((int32_t)modelSize.y);

                    auto patchPos = m_DataStream->Tell() + offsetof(SMagicaVoxelChunkHeader, ChunkContentSize);

                    SMagicaVoxelChunkHeader xyziChunk = { XYZI_CHUNK_ID, 0, 0 };
                    m_DataStream->Write(xyziChunk);
                    m_DataStream->Write((int32_t)0);

                    ankerl::unordered_dense::map<Math::Vec3i, const CChunk*, Math::Vec3iHasher> chunks;
                    for (int mx = 0; mx <= modelSize.x; mx += Config::ChunkSize)
                    {
                        for (int my = 0; my <= modelSize.y; my += Config::ChunkSize)
                        {
                            for (int mz = 0; mz <= modelSize.z; mz += Config::ChunkSize)
                            {
                                auto chunkpos = GetChunkpos(Math::Vec3i(bbox.End.x - (mx + x), y + my, z + mz));
                                auto chunk = p_Model->GetChunk(chunkpos);
                                if(chunk)
                                    chunks[chunkpos] = chunk;
                            }
                        }
                    }

                    int32_t voxelCount = 0;
                    for (auto &&chunk : chunks)
                    {
                        auto innerBBox = chunk.second->inner_bbox(chunk.first);
                        for (int mx = innerBBox.End.x; mx >= innerBBox.Beg.x; mx--)
                        {
                            for (int my = innerBBox.Beg.y; my <= innerBBox.End.y; my++)
                            {
                                for (int mz = innerBBox.Beg.z; mz <= innerBBox.End.z; mz++)
                                {
                                    // auto relpos = chunk.first - (chunk.first & 256);

                                    // Mirrors the model for MagicaVoxel
                                    auto voxel = chunk.second->find(Math::Vec3i(mx, my, mz));
                                    if(voxel.IsInstantiated())
                                    {
                                        voxelCount++;
                                        m_DataStream->Write((uint8_t)mx);//((mx - innerBBox.Beg.x) + relpos.x));//(uint8_t)(modelSize.x - ((mx - innerBBox.Beg.x) + relpos.x)));
                                        m_DataStream->Write((uint8_t)mz);//((mz - innerBBox.Beg.z) + relpos.z));
                                        m_DataStream->Write((uint8_t)my);//((my - innerBBox.Beg.y) + relpos.y));

                                        auto mapIt = m_VoxelIndexMap.find((uint32_t)voxel);
                                        if(mapIt == m_VoxelIndexMap.end())
                                        {
                                            m_ColorPalette[m_VoxelIndex - 1].FromRGBA(0xFF000000 | voxel.GetColor());
                                            m_Materials.push_back(voxel.GetMaterial());

                                            // auto mat = MaterialManager::GetMaterial(voxel.GetMaterial());
                                            // if(mat)
                                            //     m_Materials.push_back(mat);
                                            // else
                                            //     m_Materials.push_back(MaterialManager::GetMaterial(0));

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
    }

    void CMagicaVoxelFormat::TraverseVCoreSceneTree()
    {
        // TODO: 
        // auto rootGroup = std::static_pointer_cast<SGroupNode>(m_MagicaSceneTree[m_MagicaSceneTree.size() - 1]);
        // if(SceneTree)
        //     rootGroup->ChildrensID.push_back(TraverseSceneTreeNode(SceneTree));
    }

    int CMagicaVoxelFormat::TraverseSceneTreeNode(const CSceneNode* p_Node)
    {
        // TODO: 
        // if(p_Node->Model)
        // {
        //     auto childId = WriteModel(p_Node->Model);
        //     auto translate = std::static_pointer_cast<STransformNode>(m_MagicaSceneTree[childId]);
        //     translate->Frames[0].Translation += p_Node->Position + p_Node->Model->CalculateBBox().GetSize() * 0.5f;
        //     if(!p_Node->Visible)
        //         translate->Attributes["_hidden"] = "1";
        //     return childId;
        // }
        // else if(p_Node->Animation)
        // {
        //     auto childId = WriteAnimation(p_Node->Animation);
        //     auto translate = std::static_pointer_cast<STransformNode>(m_MagicaSceneTree[childId]);
        //     translate->Frames[0].Translation += p_Node->Position + p_Node->Animation->GetFrame(0).Model->CalculateBBox().GetSize() * 0.5f;
        //     if(!p_Node->Visible)
        //         translate->Attributes["_hidden"] = "1";
        //     return childId;
        // }

        // auto transform = std::make_shared<STransformNode>();
        // transform->NodeID = m_MagicaSceneTree.size();
        // transform->ChildID = m_MagicaSceneTree.size() + 1;
        // transform->LayerID = 0;
        // transform->NumFrames = 1;
        // transform->Frames.push_back(SFrameTransform());
        // transform->Frames[0].Translation = p_Node->Position; //Math::Vec3i(_Node->Position.x, _Node->Position.z, _Node->Position.y);
        // // transform->Frames[0].Rotation = Math::Vec3i(_Node->Rotation.x, _Node->Rotation.z, _Node->Rotation.y);
        // m_MagicaSceneTree.push_back(transform);

        // auto group = std::make_shared<SGroupNode>();
        // group->NodeID = m_MagicaSceneTree.size();
        // m_MagicaSceneTree.push_back(group);

        // for (auto &&node : *p_Node)
        //     group->ChildrensID.push_back(TraverseSceneTreeNode(node));

        // return transform->NodeID;
    }

    void CMagicaVoxelFormat::WriteSceneTree()
    {
        

        // for (auto &&node : m_MagicaSceneTree)
        // {
        //     // Position to path later.
        //     auto patchPos = m_DataStream->Tell() + offsetof(SMagicaVoxelChunkHeader, ChunkContentSize);

        //     switch (node->Type)
        //     {
        //         case NodeType::TRANSFORM: {
        //             auto transform = std::static_pointer_cast<STransformNode>(node);

        //             SMagicaVoxelChunkHeader nTRNChunk = { {'n', 'T', 'R', 'N'}, 0, 0 };
        //             m_DataStream->Write(nTRNChunk);
        //             m_DataStream->Write(transform->NodeID);
        //             m_DataStream->Write((int32_t)transform->Attributes.size());
        //             for (auto &&dict : transform->Attributes)
        //                 WriteDictKeyString(dict.first.c_str(), dict.second.c_str());
        //             m_DataStream->Write(transform->ChildID);
        //             m_DataStream->Write((int32_t)-1);
        //             m_DataStream->Write(transform->LayerID);
        //             m_DataStream->Write(transform->NumFrames);

        //             for (auto &&frame : transform->Frames)
        //             {
        //                 m_DataStream->Write((int32_t)1);
        //                 WriteDictKeyVec3i("_t", frame.Translation);
        //                 // WriteDictKeyInt("_f", frame.FrameIdx);
        //             }

        //             if(transform->Frames.size() == 0)
        //                 m_DataStream->Write((int32_t)0);
        //         } break;
            
        //         case NodeType::GROUP: {
        //             auto group = std::static_pointer_cast<SGroupNode>(node);

        //             SMagicaVoxelChunkHeader nGRPChunk = { {'n', 'G', 'R', 'P'}, 0, 0 };
        //             m_DataStream->Write(nGRPChunk);
        //             m_DataStream->Write(group->NodeID);
        //             m_DataStream->Write((int32_t)0);
        //             m_DataStream->Write((int32_t)group->ChildrensID.size());

        //             for (auto &&child : group->ChildrensID)
        //                 m_DataStream->Write((int32_t)child);
        //         } break;

        //         case NodeType::SHAPE: {
        //             auto shape = std::static_pointer_cast<SShapeNode>(node);

        //             SMagicaVoxelChunkHeader nSHPChunk = { {'n', 'S', 'H', 'P'}, 0, 0 };
        //             m_DataStream->Write(nSHPChunk);
        //             m_DataStream->Write(shape->NodeID);
        //             // m_DataStream->Write((int32_t)0);

        //             m_DataStream->Write((int32_t)shape->Attributes.size());
        //             for (auto &&dict : shape->Attributes)
        //                 WriteDictKeyString(dict.first.c_str(), dict.second.c_str());

        //             m_DataStream->Write((int32_t)shape->Models.size());

        //             for (auto &&model : shape->Models)
        //             {
        //                 m_DataStream->Write((int32_t)model.ModelId);

        //                 if(shape->Models.size() == 1)
        //                     m_DataStream->Write((int32_t)0);
        //                 else
        //                 {
        //                     m_DataStream->Write((int32_t)1);
        //                     WriteDictKeyInt("_f", model.FrameIdx);
        //                 }
        //             }
        //         } break;
        //     }

        //     auto currentPos = m_DataStream->Tell();
        //     m_DataStream->Seek(patchPos, SeekOrigin::BEG);
        //     m_DataStream->Write((int32_t)(currentPos - (patchPos + 8)));
        //     m_DataStream->Seek(currentPos, SeekOrigin::BEG);
        // }
    }

    //////////////////////////////////////////////////
    // Reading functions
    //////////////////////////////////////////////////

    void CMagicaVoxelFormat::ParseFormat()
    {   
        std::string signature(4, '\0');
        m_DataStream->Read(&signature[0], 4);
        signature += "\0";

        // Checks the file header
        if(signature != "VOX ")
            throw CVoxelFormatException("Unknown file format");

        int Version = m_DataStream->Read<int>();
        if(Version < 150)
            throw CVoxelFormatException("Version: " + std::to_string(Version) + " is not supported");
          
        LoadMaterials(); 
        ProcessChunks();
    }

    void CMagicaVoxelFormat::LoadColorPalette()
    {
        if(!m_DataStream->Eof())
        {
            auto position = m_DataStream->Tell();

            SMagicaVoxelChunkHeader chunk = m_DataStream->Read<SMagicaVoxelChunkHeader>();
            if(chunk.Id == MAIN_CHUNK_ID)
            {
                bool found = false;

                while (!m_DataStream->Eof() && !found)
                {
                    chunk = m_DataStream->Read<SMagicaVoxelChunkHeader>();
                    switch (chunk.Id) 
                    {
                        case RGBA_CHUNK_ID: 
                        {
                            m_DataStream->Read((char*)m_ColorPalette, sizeof(m_ColorPalette));
                            found = true;
                        } break;

                        // Skips unknown chunks, or chunks which are not relevant for vcore.
                        default: m_DataStream->Seek(chunk.ChunkContentSize + chunk.ChildChunkSize); break;
                    }
                }
            }

            // Reset stream.
            m_DataStream->Seek(position, SeekOrigin::BEG);
        }
    }

    void CMagicaVoxelFormat::LoadMaterials()
    {
        if(!m_DataStream->Eof())
        {
            auto pos = m_DataStream->Tell();

            SMagicaVoxelChunkHeader chunk = m_DataStream->Read<SMagicaVoxelChunkHeader>();
            if(chunk.Id == MAIN_CHUNK_ID)
            {
                while (!m_DataStream->Eof())
                {
                    chunk = m_DataStream->Read<SMagicaVoxelChunkHeader>();
                    switch (chunk.Id)
                    {
                        case MATL_CHUNK_ID: ProcessMaterial(); break;
                        default: m_DataStream->Seek(chunk.ChunkContentSize + chunk.ChildChunkSize); break;
                    }
                }
            }

            m_DataStream->Seek(pos, SeekOrigin::BEG);
        }
    }

    void CMagicaVoxelFormat::ProcessChunks()
    {
        START_PROFILER("LoadColorPalette", TimeUnit::NANO)           
        // Preloads the color palette.
        LoadColorPalette();
        END_PROFILER()

        if(!m_DataStream->Eof())
        {
            SMagicaVoxelChunkHeader chunk = m_DataStream->Read<SMagicaVoxelChunkHeader>();
            if(chunk.Id == MAIN_CHUNK_ID)
            {
                SMagicaVoxelSceneTreeHelper treeHelper;

                std::shared_ptr<uint32_t[]> colorpalette(new uint32_t[sizeof(m_ColorPalette) / sizeof(m_ColorPalette[0])]);
                memcpy(colorpalette.get(), m_ColorPalette, sizeof(m_ColorPalette));

                bool firstGroup = true;

                while (!m_DataStream->Eof())
                {
                    chunk = m_DataStream->Read<SMagicaVoxelChunkHeader>();
                    switch (chunk.Id) 
                    {
                        case SIZE_CHUNK_ID: 
                        {
                            // START_PROFILER("ProcessModel", TimeUnit::NANO)
                            ProcessModel(colorpalette);
                            // END_PROFILER()
                        } break;

                        case TRANSFORM_CHUNK_ID:
                        {
                            auto transform = ProcessTransformNode();
                            treeHelper.ParentTransforms[transform.ChildId] = std::move(transform);
                        } break;

                        case GROUP_CHUNK_ID:
                        {
                            auto group = ProcessGroupNode();

                            CSceneNodeBase *node = SceneTree.get();
                            if(!firstGroup) [[likely]]
                                node = new CSceneNode(nullptr);
                            else
                                firstGroup = false;

                            treeHelper.BuildTree(group.NodeId, node);
                            treeHelper.BuildTree(node, group.Children);
                        } break;

                        case SHAPE_CHUNK_ID:
                        {
                            auto shape = ProcessShapeNode();
                            CSceneNodeBase *node = nullptr;

                            if(shape.Models.size() > 1)
                                node = new CSceneAnimationNode(nullptr, std::move(shape.Models));
                            else
                                node = new CSceneModelNode(nullptr,shape.Models[0].ModelId);

                            treeHelper.BuildTree(shape.NodeId, node);
                        } break;

                        // Skips unknown chunks, or chunks which are not relevant for vcore.
                        default: m_DataStream->Seek(chunk.ChunkContentSize + chunk.ChildChunkSize); break;
                    }
                }

                // Adds the remaining orphans to the root of the tree.
                for (auto &&orphan : treeHelper.Orphans) 
                    SceneTree->AddChild(orphan.second);
            }
        }
    }

    void CMagicaVoxelFormat::ProcessMaterial()
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

        if(!m_MaterialMap)
            m_MaterialMap = std::make_shared<MaterialMap>();

        if(materialIdx < 257)
        {
            auto &matConfig =  m_MaterialMap->Materials[materialIdx];
            matConfig.first = material;
            matConfig.second = 0xFF;
        }
    }

    void CMagicaVoxelFormat::ProcessModel(const std::shared_ptr<uint32_t[]> &p_Colorpalette)
    {
        VoxelModel model;
        Math::Vec3f size;
        if(static_cast<unsigned>(m_Mode) & static_cast<unsigned>(FileMode::STREAMED))
        {
            model = std::make_shared<CVoxelSpace>(new CMagicaVoxelStreamable(m_IOHandler, p_Colorpalette, m_DataStream->Tell(), m_MaterialMap, m_DataStream->GetFilePath()));
            size = CMagicaVoxelModelParser::ProcessSize(m_DataStream);
        }
        else
        {
            model = std::make_shared<CVoxelSpace>();
            CMagicaVoxelModelParser parser(m_DataStream, p_Colorpalette, m_DataStream->Tell(), m_MaterialMap);
            parser.FillVoxelSpace(*model);
            size = parser.GetSize();
        }

        SceneTree->AddModel(model);
        model->Origin = (size * 0.5);
    }

    STransformNode CMagicaVoxelFormat::ProcessTransformNode()
    {
        STransformNode ret;

        ret.NodeId = m_DataStream->Read<uint32_t>();
        
        // Read the attributes.
        int keys = m_DataStream->Read<int>();
        for (int i = 0; i < keys; i++)
        {
            uint32_t size = m_DataStream->Read<uint32_t>();
            std::string key(size, '\0'); 
            m_DataStream->Read(&key[0], size);
            if(key == "_name")
            {              
                size = m_DataStream->Read<uint32_t>();
                std::string value(size, '\0'); 
                m_DataStream->Read(&value[0], size);
                ret.Name = value;
            }
            else if(key == "_hidden")
            {
                size = m_DataStream->Read<uint32_t>();
                char hidden = 0;
                m_DataStream->Read(&hidden, size);
                ret.Hidden = hidden;
            }
            else
            {
                size = m_DataStream->Read<int>();
                m_DataStream->Seek(size);
            }            
        }

        ret.ChildId = m_DataStream->Read<uint32_t>();

        // Skip Reverse and Layer
        m_DataStream->Seek(sizeof(uint32_t) * 2);

        uint32_t frames = m_DataStream->Read<uint32_t>();
        for (uint32_t i = 0; i < frames; i++)
        {
            SFrameTransform frameTransform;

            keys = m_DataStream->Read<uint32_t>();
            for (int j = 0; j < keys; j++)
            {
                uint32_t size = m_DataStream->Read<uint32_t>();
                std::string key(size, '\0'); 
                m_DataStream->Read(&key[0], size);

                if(key == "_t")
                {
                    size = m_DataStream->Read<uint32_t>();
                    std::string value(size, '\0'); 
                    m_DataStream->Read(&value[0], size);

                    frameTransform.Translation = ParsePosition(value);
                    frameTransform.Translation.x *= -1;
                }
                else if(key == "_r")
                {
                    size = m_DataStream->Read<uint32_t>();
                    std::string value(size, '\0'); 
                    m_DataStream->Read(&value[0], size);

                    int rot = std::stoi(value);

                    uint8_t row1Idx = rot & 3;
                    uint8_t row2Idx = (rot >> 2) & 3;
                    uint8_t row3Idx = 3 - (row1Idx + row2Idx);

                    auto rotation = Math::Mat4x4(Math::Vec4f(0, 0, 0, 0),
                                            Math::Vec4f(0, 0, 0, 0),
                                            Math::Vec4f(0, 0, 0, 0),
                                            Math::Vec4f(0, 0, 0, 1));

                    rotation.x.v[row1Idx] = ((rot & 0x10) == 0x10) ? -1 : 1;
                    rotation.y.v[row2Idx] = ((rot & 0x20) == 0x20) ? -1 : 1;
                    rotation.z.v[row3Idx] = ((rot & 0x40) == 0x40) ? -1 : 1;

                    // Gets the euler angle, y is the up axis.
                    frameTransform.Rotation = rotation.GetEuler();
                    frameTransform.Scale = rotation.GetScale();
                    std::swap(frameTransform.Rotation.y, frameTransform.Rotation.z);
                    std::swap(frameTransform.Scale.y, frameTransform.Scale.z);
                }
                else if(key == "_f")
                {
                    int size = m_DataStream->Read<uint32_t>();
                    std::string value(size, '\0'); 
                    m_DataStream->Read(&value[0], size);

                    frameTransform.FrameIdx = std::stoi(value);
                }
            }

            ret.Frames.push_back(frameTransform);
        }

        return ret;
    }
    
    SGroupNode CMagicaVoxelFormat::ProcessGroupNode()
    {
        SGroupNode ret;

        ret.NodeId = m_DataStream->Read<uint32_t>();
        CMagicaVoxelDictionary(m_DataStream).SkipDict();

        uint32_t children = m_DataStream->Read<uint32_t>();
        for (uint32_t i = 0; i < children; i++)
            ret.Children.push_back(m_DataStream->Read<uint32_t>());

        return ret;
    }

    SShapeNode CMagicaVoxelFormat::ProcessShapeNode()
    {
        SShapeNode ret;

        ret.NodeId = m_DataStream->Read<uint32_t>();
        CMagicaVoxelDictionary(m_DataStream).SkipDict();

        uint32_t children = m_DataStream->Read<uint32_t>();
        for (uint32_t i = 0; i < children; i++)
        {
            CSceneAnimationNode::SFrame frame;
            frame.ModelId = m_DataStream->Read<uint32_t>();

            // Skips the dictionary
            uint32_t keys = m_DataStream->Read<uint32_t>();
            for (uint32_t i = 0; i < keys; i++)
            {
                uint32_t size = m_DataStream->Read<uint32_t>();
                std::string key(size, '\0'); 
                m_DataStream->Read(&key[0], size);

                if(key == "_f")
                {
                    uint32_t size = m_DataStream->Read<uint32_t>();
                    std::string value(size, '\0'); 
                    m_DataStream->Read(&value[0], size);

                    frame.FrameIdx = std::stoi(value);
                }
                else
                    m_DataStream->Seek(m_DataStream->Read<uint32_t>());
            }

            ret.Models.push_back(frame);
        }

        return ret;
    }
}
