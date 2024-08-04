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


#include <fstream>

#include <iostream>
#include <iomanip>

#include <stb_image.h>
#include <string.h>
#include <VCore/Misc/Exceptions.hpp>
#include "QubicleFormat.hpp"

using namespace std;

namespace VCore
{
    void CQubicleFormat::ParseFormat()
    {
        m_Models.clear();
        m_Materials.clear();
        m_Materials.clear();
        m_Textures.clear();

        m_Materials.push_back(std::make_shared<CMaterial>());

        std::string Signature(4, '\0');
        m_DataStream->Read(&Signature[0], 4);
        Signature += "\0";

        // Checks the file header
        if(Signature != "QBCL")
            throw CVoxelLoaderException("Unknown file format");

        m_DataStream->Seek(4);    // Program version e.g. 3.1.2.0

        int version = m_DataStream->Read<int>();
        if(version != 2)   // Version 2.0
            throw CVoxelLoaderException("Unsupported version!");

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
        LoadNode();

        for (auto &&m : m_Models)
            m->Textures = m_Textures;     
    }

    void CQubicleFormat::LoadNode()
    {
        uint32_t type = m_DataStream->Read<uint32_t>();
        m_DataStream->Seek(sizeof(int));  // I dont know.
        // uint32_t size = m_DataStream->Read<uint32_t>();

        switch (type)
        {
            case 0: // Matrix
            {
                LoadMatrix();
            }break;

            case 1: // Model
            {
                LoadModel();
            }break;
        
            default:
            {
                throw CVoxelLoaderException("Unknown type: " + std::to_string(type));
                // m_DataStream->Seek(size);
            } break;
        }
    }

    void CQubicleFormat::LoadModel()
    {
        uint32_t size = m_DataStream->Read<uint32_t>();
        m_DataStream->Seek(size);
        m_DataStream->Seek(39);   // I dont know for which this chunk is for. It's always the same.

        uint32_t childCount = m_DataStream->Read<uint32_t>();
        for (uint32_t i = 0; i < childCount; i++)
            LoadNode();        
    }

    void CQubicleFormat::LoadMatrix()
    {
        uint32_t nameLen = m_DataStream->Read<uint32_t>();
        std::string name(nameLen + 1, '\0');
        m_DataStream->Read(&name[0], nameLen);
        m_DataStream->Seek(3); //Mysterious 3 bytes always 0x01 0x01 0x00

        VoxelModel mesh = std::make_shared<CVoxelModel>();
        mesh->Materials = m_Materials;
        mesh->Name = name;
        auto size = ReadVector();

        auto pos = ReadVector();
        // auto halfSize = (size / 2.0);
        // pos += halfSize;

        auto sceneNode = std::make_shared<CSceneNode>();
        sceneNode->Position = pos;
        sceneNode->Mesh = mesh;
        m_SceneTree->AddChild(sceneNode);
        m_DataStream->Seek(3 * sizeof(float));    //Pivot position.

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

                    for (uint8_t j = 0; j < c.R; j++)
                    {
                        Math::Vec3i pos;

                        pos.z = index % (uint32_t)size.z;
                        pos.x = (uint32_t)(index / (uint32_t)size.z);
                        pos.y = y;
                        
                        AddVoxel(mesh, data, pos);
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
                    
                    AddVoxel(mesh, data, pos);
                    y++;
                }
            }

            index++;
        }
        free(Data);

        m_Models.push_back(mesh);
    }

    void CQubicleFormat::LoadCompound()
    {

    }

    Math::Vec3i CQubicleFormat::ReadVector()
    {
        Math::Vec3i ret;

        ret.x = m_DataStream->Read<int>();
        ret.y = m_DataStream->Read<int>();
        ret.z = m_DataStream->Read<int>();

        return ret;
    }

    int CQubicleFormat::GetColorIdx(int color)
    {
        int ret = 0;
        CColor c;
        c.FromRGBA(color);

        if(c.A == 0)
            return -1;

        c.A = 255;
        color = c.AsRGBA();

        auto IT = m_ColorIdx.find(color);
        if(IT == m_ColorIdx.end())
        {
            auto texIT = m_Textures.find(TextureType::DIFFIUSE);
            if(texIT == m_Textures.end())
                m_Textures[TextureType::DIFFIUSE] = std::make_shared<CTexture>();

            m_Textures[TextureType::DIFFIUSE]->AddPixel(c);
            ret = m_Textures[TextureType::DIFFIUSE]->GetSize().x - 1;

            m_ColorIdx.insert({color, ret});
        }
        else
            ret = IT->second;

        return ret;
    }

    void CQubicleFormat::AddVoxel(VoxelModel mesh, int color, Math::Vec3i pos)
    {
        int cid = GetColorIdx(color);
        if(cid == -1)
            return;

        mesh->SetVoxel(pos, 0, cid, false);
    }
}
