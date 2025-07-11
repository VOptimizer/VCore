#ifndef MAGICAVOXELSCENETREEWRITER_HPP
#define MAGICAVOXELSCENETREEWRITER_HPP

#include "MagicaVoxelDictionary.hpp"
#include "MagicaVoxelFormat.hpp"
#include "MagicaVoxelModelParser.hpp"
#include "VCore/Math/Vector.hpp"
#include <VCore/Misc/fast_vector.hpp>
#include <VCore/Misc/unordered_dense.h>
#include <VCore/Misc/FileStream.hpp>
#include <VCore/Voxel/Storage/VoxelSpace.hpp>
#include <VCore/Formats/SceneNode.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace VCore 
{
    class CMagicaVoxelScenetreeWriter : public ISceneTreeVisitor<VoxelModel>
    {
        public:
            CMagicaVoxelScenetreeWriter(
                const std::shared_ptr<TSceneTree<VoxelModel>> &p_SceneTree,
                IFileStream *p_DataStream
            ) : ISceneTreeVisitor<VoxelModel>(p_SceneTree), m_DataStream(p_DataStream), m_Dict(m_DataStream) {}

            ankerl::unordered_dense::map<uint32_t, fast_vector<uint32_t>> VoxelModelMap;

            void WriteTree() { TraverseTree(); }
        protected:
            void TraverseTree() override 
            { 
                auto node = m_SceneTree.get();
                EnterSceneNode(node);
                LeaveSceneNode(node);
            }

            void EnterSceneNode(const CSceneNodeBase *p_Node) override
            {
                auto model = dynamic_cast<const CSceneModelNode*>(p_Node);
                Math::Vec3f origin;
                if(model)
                    origin = Math::round(m_SceneTree->GetModels()[model->ModelId]->Origin);

                WriteTransformNode(p_Node, origin, model ? model->Name : "");

                if(model)
                    WriteModel(model);
                else
                    WriteGroupNode(p_Node);
            }

            void LeaveSceneNode(const CSceneNodeBase *) override {  }
        private:
            void WriteModel(const CSceneModelNode *p_Model)
            {
                auto map = VoxelModelMap[p_Model->ModelId];
                if(map.size() == 1)
                    WriteShapeNode(map[0]);
                else
                {

                }
            }
            
            void WriteTransformNode(const CSceneNodeBase *p_Node, const Math::Vec3f &p_Origin, std::string_view p_Name)
            {
                SMagicaVoxelChunkHeader nTRNChunk = { MakeChunkId('n', 'T', 'R', 'N'), 0, 0 };
                m_DataStream->Write(nTRNChunk);
                
                auto startPos = m_DataStream->Tell();
                m_DataStream->Write(m_NodeId++);

                fast_vector<std::pair<std::string, std::string>> attributes = {
                    {"_hidden", p_Node->Visible ? "0" : "1"}
                };

                if(!p_Name.empty())
                    attributes.push_back({"_name", p_Name.data()});

                m_DataStream->Write((int32_t)attributes.size());
                for (auto &&attribute : attributes)
                    m_Dict.WriteDictKeyString(attribute.first.c_str(), attribute.second.c_str());

                m_DataStream->Write(m_NodeId);
                m_DataStream->Write((int32_t)-1);
                m_DataStream->Write((int32_t)(p_Node == m_SceneTree.get() ? -1 : 0));
                m_DataStream->Write((int32_t)1);

                m_DataStream->Write((int32_t)1);
                m_Dict.WriteDictKeyVec3i("_t", p_Node->GetPosition() + p_Origin);

                PatchSize(startPos);
            }

            void WriteGroupNode(const CSceneNodeBase *p_Node)
            {
                SMagicaVoxelChunkHeader nGRPChunk = { MakeChunkId('n', 'G', 'R', 'P'), 0, 0 };
                m_DataStream->Write(nGRPChunk);
                
                auto startPos = m_DataStream->Tell();
                m_DataStream->Write(m_NodeId++);
                m_DataStream->Write((int32_t)0);
                m_DataStream->Write((int32_t)p_Node->GetChildrenCount());

                auto arrayPos = m_DataStream->Tell();
                for (uint32_t i = 0; i < p_Node->GetChildrenCount(); i++) 
                    m_DataStream->Write((int32_t)0);

                PatchSize(startPos);

                for (auto &&node : *p_Node) 
                {
                    m_DataStream->Seek(arrayPos, SeekOrigin::BEG);
                    m_DataStream->Write(m_NodeId);
                    m_DataStream->Seek(0, SeekOrigin::END);
                    arrayPos += sizeof(m_NodeId);

                    EnterSceneNode(node);
                    LeaveSceneNode(node);
                }
            }

            void WriteShapeNode(uint32_t p_ModelId)
            {
                SMagicaVoxelChunkHeader nSHPChunk = { MakeChunkId('n', 'S', 'H', 'P'), 0, 0 };
                m_DataStream->Write(nSHPChunk);
                
                auto startPos = m_DataStream->Tell();
                m_DataStream->Write(m_NodeId++);
                m_DataStream->Write((int32_t)0);
                m_DataStream->Write((int32_t)1);

                m_DataStream->Write((int32_t)p_ModelId);
                m_DataStream->Write((int32_t)0);

                PatchSize(startPos);
            }

            void PatchSize(uint64_t p_StartPos)
            {
                // Patch the chunk size.
                auto currentPos = m_DataStream->Tell();
                m_DataStream->Seek(p_StartPos - sizeof(int32_t) * 2, SeekOrigin::BEG);
                m_DataStream->Write(currentPos - p_StartPos);
                m_DataStream->Seek(0, SeekOrigin::END);
            }

            IFileStream *m_DataStream;
            CMagicaVoxelDictionary m_Dict;
            uint32_t m_NodeId{};
    };
}

#endif