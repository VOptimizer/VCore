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
#include <string.h>
#include <VCore/Misc/Exceptions.hpp>
#include "QubicleBinaryTreeFormat.hpp"

namespace VCore
{
    void CQubicleBinaryTreeFormat::ParseFormat()
    {
        if(m_DataStream->Read<int>() != 0x32204251)
            throw CVoxelLoaderException("Unknown file format");

        char major = m_DataStream->Read<char>();
        char minor = m_DataStream->Read<char>();

        if(major != 1 && minor != 0)
            throw CVoxelLoaderException("Unsupported version!");

        m_Materials.push_back(std::make_shared<CMaterial>());

        m_DataStream->Seek(3 * sizeof(float));
        m_DataStream->Seek(8); // COLORMAP
        ReadColors();

        m_DataStream->Seek(8); // DATATREE
        LoadNode();

        m_ColorIdx.clear();

        for (auto &&m : m_Models)
            m->Textures = m_Textures;        
    }

    void CQubicleBinaryTreeFormat::ReadColors()
    {
        int count = m_DataStream->Read<int>();
        m_HasColormap = count > 0;
        for (int i = 0; i < count; i++)
        {
            CColor c;
            c.FromRGBA(m_DataStream->Read<uint32_t>());

            auto texIT = m_Textures.find(TextureType::DIFFIUSE);
            if(texIT == m_Textures.end())
                m_Textures[TextureType::DIFFIUSE] = std::make_shared<CTexture>();

            m_Textures[TextureType::DIFFIUSE]->AddPixel(c);
        }
    }

    void CQubicleBinaryTreeFormat::LoadNode()
    {
        uint32_t type = m_DataStream->Read<uint32_t>();
        uint32_t size = m_DataStream->Read<uint32_t>();

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

            case 2: // Compound
            {
                LoadCompound();
            }break;
        
            default:
            {
                m_DataStream->Seek(size);
            } break;
        }
    }

    void CQubicleBinaryTreeFormat::LoadModel()
    {
        uint32_t childCount = m_DataStream->Read<uint32_t>();
        for (uint32_t i = 0; i < childCount; i++)
            LoadNode();
    }

    void CQubicleBinaryTreeFormat::LoadMatrix()
    {
        int nameLen = m_DataStream->Read<int>();
        std::string name(nameLen + 1, '\0');
        m_DataStream->Read(&name[0], nameLen);

        VoxelModel mesh = std::make_shared<CVoxelModel>();
        mesh->Materials = m_Materials;
        mesh->Name = name;
        auto pos = ReadVector();

        m_DataStream->Seek(6 * sizeof(int));
        auto size = ReadVector();
        // auto halfSize = (mesh->GetSize() / 2.0);
        // pos += halfSize;

        auto sceneNode = std::make_shared<CSceneNode>();
        sceneNode->Position = pos;
        sceneNode->Mesh = mesh;
        m_SceneTree->AddChild(sceneNode);

        uint32_t dataSize = m_DataStream->Read<uint32_t>();

        int OutSize = 0;

        std::vector<char> data(dataSize, 0);
        m_DataStream->Read(&data[0], dataSize);
        char *Data = stbi_zlib_decode_malloc(data.data(), dataSize, &OutSize);
        int strmPos = 0;

        for (uint32_t x = 0; x < (uint32_t)size.x; x++)
        {
            for (uint32_t z = 0; z < (uint32_t)size.z; z++)
            {
                for (uint32_t y = 0; y < (uint32_t)size.y; y++)
                {
                    int color;
                    memcpy(&color, Data + strmPos, sizeof(int));
                    strmPos += sizeof(int);

                    int cid;

                    if(m_HasColormap)
                        cid = color & 0xFF;
                    else
                        cid = GetColorIdx(color);

                    if(cid == -1 || ((color & 0xFF000000) >> 24) == 0)
                        continue;

                    auto pos = Math::Vec3i(x, y, z);
                    mesh->SetVoxel(pos, 0, cid, false);
                }
            }
        }
        free(Data);

        m_Models.push_back(mesh);
    }

    void CQubicleBinaryTreeFormat::LoadCompound()
    {
        int nameLen = m_DataStream->Read<int>();
        std::string name(nameLen + 1, '\0');
        m_DataStream->Read(&name[0], nameLen);

        VoxelModel mesh = std::make_shared<CVoxelModel>();
        mesh->Materials = m_Materials;
        mesh->Name = name;
        auto pos = ReadVector();

        m_DataStream->Seek(6 * sizeof(int));
        auto size = ReadVector();

        // auto halfSize = (mesh->GetSize() / 2.0);
        // pos += halfSize;

        auto sceneNode = std::make_shared<CSceneNode>();
        sceneNode->Position = pos;
        sceneNode->Mesh = mesh;
        m_SceneTree->AddChild(sceneNode);

        uint32_t dataSize = m_DataStream->Read<uint32_t>();

        int OutSize = 0;
        std::vector<char> data(dataSize, 0);
        m_DataStream->Read(&data[0], dataSize);
        char *Data = stbi_zlib_decode_malloc(data.data(), dataSize, &OutSize);
        int strmPos = 0;

        for (uint32_t x = 0; x < (uint32_t)size.x; x++)
        {
            for (uint32_t z = 0; z < (uint32_t)size.z; z++)
            {
                for (uint32_t y = 0; y < (uint32_t)size.y; y++)
                {
                    int color;
                    memcpy(&color, Data + strmPos, sizeof(int));
                    strmPos += sizeof(int);

                    int cid;

                    if(m_HasColormap)
                        cid = color & 0xFF;
                    else
                        cid = GetColorIdx(color);

                    if(cid == -1 || ((color & 0xFF000000) >> 24) == 0)
                        continue;

                    auto pos = Math::Vec3f(x, y, z);
                    mesh->SetVoxel(pos, 0, cid, false);
                }
            }
        }
        free(Data);

        m_Models.push_back(mesh);

        uint32_t childCount = m_DataStream->Read<uint32_t>();
        for (uint32_t i = 0; i < childCount; i++)
            LoadNode();
    }

    Math::Vec3i CQubicleBinaryTreeFormat::ReadVector()
    {
        Math::Vec3i ret;

        ret.x = m_DataStream->Read<int>();
        ret.y = m_DataStream->Read<int>();
        ret.z = m_DataStream->Read<int>();

        return ret;
    }

    int CQubicleBinaryTreeFormat::GetColorIdx(int color)
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
}
