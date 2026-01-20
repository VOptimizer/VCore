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
#include "QubicleBinaryTreeFormat.hpp"
#include "VCore/Formats/SceneNode.hpp"
#include "VCore/Misc/fast_vector.hpp"

namespace VCore
{
    void CQubicleBinaryTreeFormat::ParseFormat()
    {
        if(m_DataStream->Read<int>() != 0x32204251)
            throw CVoxelFormatException("Unknown file format");

        char major = m_DataStream->Read<char>();
        char minor = m_DataStream->Read<char>();

        if(major != 1 && minor != 0)
            throw CVoxelFormatException("Unsupported version!");

        m_DataStream->Seek(3 * sizeof(float));
        m_DataStream->Seek(8); // COLORMAP
        ReadColors();

        m_DataStream->Seek(8); // DATATREE
        LoadNode(SceneTree.get());

        m_Colors.clear();   
    }

    void CQubicleBinaryTreeFormat::ReadColors()
    {
        int count = m_DataStream->Read<int>();
        m_HasColormap = count > 0;
        for (int i = 0; i < count; i++)
            m_Colors.push_back(m_DataStream->Read<uint32_t>());
    }

    void CQubicleBinaryTreeFormat::LoadNode(CSceneNodeBase *p_Parent)
    {
        uint32_t type = m_DataStream->Read<uint32_t>();
        uint32_t size = m_DataStream->Read<uint32_t>();

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
                m_DataStream->Seek(size);
            } break;
        }
    }

    void CQubicleBinaryTreeFormat::LoadModel(CSceneNodeBase *p_Parent)
    {
        uint32_t childCount = m_DataStream->Read<uint32_t>();
        for (uint32_t i = 0; i < childCount; i++)
            LoadNode(p_Parent);
    }

    CSceneModelNode *CQubicleBinaryTreeFormat::LoadMatrix(CSceneNodeBase *p_Parent)
    {
        int nameLen = m_DataStream->Read<int>();
        std::string name(nameLen + 1, '\0');
        m_DataStream->Read(&name[0], nameLen);

        VoxelModel mesh = std::make_shared<CVoxelSpace>();
        auto pos = ReadVector();

        m_DataStream->Seek(6 * sizeof(int));
        auto size = ReadVector();
        // auto halfSize = (mesh->GetSize() / 2.0);
        // pos += halfSize;

        auto sceneNode = new CSceneModelNode(p_Parent, SceneTree->GetModels().size());
        sceneNode->SetPosition(pos);
        sceneNode->Name = name;
        p_Parent->AddChild(sceneNode);

        uint32_t dataSize = m_DataStream->Read<uint32_t>();

        int OutSize = 0;

        fast_vector<char> data(dataSize, 0);
        m_DataStream->Read(&data[0], dataSize);
        char *Data = stbi_zlib_decode_malloc(data.data(), dataSize, &OutSize);
        int strmPos = 0;

        for (uint32_t x = 0; x < (uint32_t)size.x; x++)
        {
            for (uint32_t z = 0; z < (uint32_t)size.z; z++)
            {
                for (uint32_t y = 0; y < (uint32_t)size.y; y++)
                {
                    uint32_t color;
                    memcpy(&color, Data + strmPos, sizeof(uint32_t));
                    strmPos += sizeof(uint32_t);

                    if(m_HasColormap)
                    {
                        if(color < m_Colors.size())
                            color = m_Colors[color];
                    }

                    if(color == 0xFFFFFFFF || ((color & 0xFF000000) >> 24) == 0)
                        continue;

                    auto pos = Math::Vec3i(x, y, z);
                    mesh->Insert({pos, CVoxel(color, 0)});
                }
            }
        }
        free(Data);

        SceneTree->AddModel(mesh);
        return sceneNode;
    }

    void CQubicleBinaryTreeFormat::LoadCompound(CSceneNodeBase *p_Parent)
    {
        auto node = LoadMatrix(p_Parent);
        uint32_t childCount = m_DataStream->Read<uint32_t>();
        for (uint32_t i = 0; i < childCount; i++)
            LoadNode(node);
    }

    Math::Vec3i CQubicleBinaryTreeFormat::ReadVector()
    {
        Math::Vec3i ret;

        ret.x = m_DataStream->Read<int>();
        ret.y = m_DataStream->Read<int>();
        ret.z = m_DataStream->Read<int>();

        return ret;
    }
}
