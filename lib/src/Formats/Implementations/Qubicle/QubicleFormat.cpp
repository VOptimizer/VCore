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

#include <stb_image.h>
#include <cstring>
#include <VCore/Misc/Exceptions.hpp>
#include <VCore/Meshing/MaterialManager.hpp>
#include "QubicleFormat.hpp"
#include "VCore/Formats/SceneNode.hpp"

namespace VCore
{
    void CQubicleFormat::ParseFormat()
    {
        std::string Signature(4, '\0');
        m_DataStream->Read(&Signature[0], 4);
        Signature += "\0";

        // Checks the file header
        if(Signature != "QBCL")
            throw CVoxelFormatException("Unknown file format");

        m_DataStream->Seek(4);    // Program version e.g. 3.1.2.0

        int version = m_DataStream->Read<int>();
        if(version != 2)   // Version 2.0
            throw CVoxelFormatException("Unsupported version!");

        //Thumbnail size
        uint32_t width = m_DataStream->Read<uint32_t>();
        uint32_t height = m_DataStream->Read<uint32_t>();

        // m_DataStream->Seeks the BGRA encoded thumbnail.
        m_DataStream->Seek(width * height * 4);

        // Metadata
        // m_DataStream->Seeks all the metadata. (Same order as in the gui)
        for (char i = 0; i < 7; i++)
        {
            uint32_t size = m_DataStream->Read<uint32_t>();
            m_DataStream->Seek(size);
        }

        m_DataStream->Seek(16);   //Timestamp?
        LoadNode(SceneTree.get());
    }

    void CQubicleFormat::LoadNode(CSceneNodeBase *p_Parent)
    {
        uint32_t type = m_DataStream->Read<uint32_t>();
        m_DataStream->Seek(sizeof(int));  // I dont know.
        // uint32_t size = m_DataStream->Read<uint32_t>();

        switch (type)
        {
            case 0: // Matrix
            {
                LoadMatrix(p_Parent);
            }break;

            case 1: // Model
            {
                LoadModel(p_Parent);
            }break;
        
            case 2: // Compound
            {
                LoadCompound(p_Parent);
            }break;

            default:
            {
                throw CVoxelFormatException("Unknown type: " + std::to_string(type));
                // m_DataStream->Seek(size);
            } break;
        }
    }

    void CQubicleFormat::LoadModel(CSceneNodeBase *p_Parent)
    {
        auto node = new CSceneNode(p_Parent);
        p_Parent->AddChild(node);

        uint32_t nameLen = m_DataStream->Read<uint32_t>();
        std::string name(nameLen + 1, '\0');
        m_DataStream->Read(&name[0], nameLen);
        node->Name = name;

        m_DataStream->Seek(39);   // I dont know for which this chunk is for. It's always the same.

        uint32_t childCount = m_DataStream->Read<uint32_t>();
        for (uint32_t i = 0; i < childCount; i++)
            LoadNode(node);        
    }

    CSceneModelNode *CQubicleFormat::LoadMatrix(CSceneNodeBase *p_Parent)
    {
        uint32_t nameLen = m_DataStream->Read<uint32_t>();
        std::string name(nameLen + 1, '\0');
        m_DataStream->Read(&name[0], nameLen);
        m_DataStream->Seek(3); //Mysterious 3 bytes always 0x01 0x01 0x00

        VoxelModel mesh = std::make_shared<CVoxelSpace>();

        auto size = ReadVector<int>();

        auto pos = ReadVector<int>();
        // auto halfSize = (size / 2.0);
        // pos += halfSize;

        auto sceneNode = new CSceneModelNode(p_Parent, SceneTree->GetModels().size());
        sceneNode->SetPosition(pos);
        p_Parent->AddChild(sceneNode);
        // mesh->Origin = ReadVector<float>(); //Pivot position.
        ReadVector<float>();

        uint32_t dataSize = m_DataStream->Read<uint32_t>();

        int OutSize = 0;
        std::vector<char> data(dataSize, 0);
        m_DataStream->Read(&data[0], dataSize);
        char *Data = stbi_zlib_decode_malloc(data.data(), dataSize, &OutSize);
        int strmPos = 0;

        uint32_t index = 0;
        while(strmPos < OutSize)
        {
            uint32_t y = 0; 
            uint16_t dataSize;

            memcpy(&dataSize, Data + strmPos, sizeof(uint16_t));
            strmPos += sizeof(uint16_t);
            
            for (size_t i = 0; i < dataSize; i++)
            {
                uint32_t data;
                memcpy(&data, Data + strmPos, sizeof(uint32_t));
                strmPos += sizeof(uint32_t);

                CColor c;
                c.FromRGBA(data);
                if(c.A == 2)    //RLE
                {
                    memcpy(&data, Data + strmPos, sizeof(uint32_t));
                    strmPos += sizeof(uint32_t);

                    CColor c2;
                    c2.FromRGBA(data);
                    for (uint8_t j = 0; j < c.R; j++)
                    {
                        Math::Vec3i pos;

                        pos.z = index % (uint32_t)size.z;
                        pos.x = (uint32_t)(index / (uint32_t)size.z);
                        pos.y = y;
                        
                        if(c2.A != 0)
                            mesh->Insert({pos, CVoxel(data, 0)});
                        y++;
                    }
                    
                    i++;
                }
                else
                {
                    Math::Vec3i pos;

                    pos.z = index % (uint32_t)size.z;
                    pos.x = (uint32_t)(index / (uint32_t)size.z);
                    pos.y = y;
                    
                    if(c.A != 0)
                        mesh->Insert({pos, CVoxel(data, 0)});
                    y++;
                }
            }

            index++;
        }
        free(Data);

        SceneTree->AddModel(mesh);

        return sceneNode;
    }

    void CQubicleFormat::LoadCompound(CSceneNodeBase *p_Parent)
    {
        auto node = LoadMatrix(p_Parent);
        uint32_t childCount = m_DataStream->Read<uint32_t>();
        for (uint32_t i = 0; i < childCount; i++)
            LoadNode(node);     
    }
}
