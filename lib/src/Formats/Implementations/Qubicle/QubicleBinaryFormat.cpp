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

#include <stdint.h>
#include <VoxelOptimizer/Misc/Exceptions.hpp>
#include "QubicleBinaryFormat.hpp"

namespace VoxelOptimizer
{
    const static int CODEFLAG = 2;
	const static int NEXTSLICEFLAG = 6;

    void CQubicleBinaryFormat::ParseFormat()
    {
        m_Header = ReadData<SQubicleBinaryHeader>();
        if(m_Header.Version[0] != 1 || m_Header.Version[1] != 1 || m_Header.Version[2] != 0 || m_Header.Version[3] != 0)
            throw CVoxelLoaderException("Version: " + std::to_string(m_Header.Version[0]) + "." + std::to_string(m_Header.Version[1]) + "." + std::to_string(m_Header.Version[2]) + "." + std::to_string(m_Header.Version[3]) + " is not supported");

        m_Materials.push_back(std::make_shared<CMaterial>());

        for (int i = 0; i < m_Header.MatrixCount; i++)
        {
            VoxelMesh mesh = VoxelMesh(new CVoxelMesh());
            mesh->Materials = m_Materials;

            uint8_t nameLen = ReadData<uint8_t>();
            std::string name(nameLen + 1, '\0');
            ReadData(&name[0], nameLen);

            mesh->Name = name;
            mesh->SetSize(ReadVector());
            auto pos = ReadVector();

            auto halfSize = (mesh->GetSize() / 2.0);

            pos += halfSize;
            std::swap(pos.y, pos.z);

            if(m_Header.ZAxisOrientation == 1)
                pos.z *= -1;

            auto sceneNode = SceneNode(new CSceneNode());
            sceneNode->SetPosition(pos);
            sceneNode->SetMesh(mesh);
            m_SceneTree->AddChild(sceneNode);

            if(m_Header.Compression == 0)
                ReadUncompressed(mesh);
            else
                ReadRLECompressed(mesh);

            mesh->GenerateVisibilityMask();
            m_Models.push_back(mesh);
        }

        m_ColorIdx.clear();
        for (auto &&m : m_Models)
            m->Colorpalettes = m_Textures;
    }

    Math::Vec3f CQubicleBinaryFormat::ReadVector()
    {
        Math::Vec3f ret;

        ret.x = ReadData<int>();
        ret.z = ReadData<int>();
        ret.y = ReadData<int>();

        return ret;
    }

    void CQubicleBinaryFormat::ReadUncompressed(VoxelMesh mesh)
    {
        Math::Vec3f Beg(1000, 1000, 1000), End;
        for (uint32_t z = 0; z < (uint32_t)mesh->GetSize().y; z++)
        {
            for (uint32_t y = 0; y < (uint32_t)mesh->GetSize().z; y++)
            {
                for (uint32_t x = 0; x < (uint32_t)mesh->GetSize().x; x++)
                {
                    int color = ReadData<int>();
                    int cid = GetColorIdx(color);
                    if(cid == -1)
                        continue;

                    auto pos = Math::Vec3f(x, z, y);

                    if(m_Header.ZAxisOrientation == 0)
                        pos.y = (uint32_t)(mesh->GetSize().y - 1) - pos.y;

                    Beg = Beg.min(pos);
                    End = End.max(pos);

                    mesh->SetVoxel(pos, 0, cid, false);
                }
            }
        }

        mesh->BBox = CBBox(Beg, End);
    }

    void CQubicleBinaryFormat::ReadRLECompressed(VoxelMesh mesh)
    {
        Math::Vec3f Beg(1000, 1000, 1000), End;
        for (uint32_t z = 0; z < (uint32_t)mesh->GetSize().y; z++)
        {
            uint32_t index = 0;

            while (true)
            {
                uint32_t data = ReadData<uint32_t>();
                int cid = 0;

                if(data == NEXTSLICEFLAG)
                    break;
                else if(data == CODEFLAG)
                {
                    uint32_t count = ReadData<uint32_t>();
                    data = ReadData<uint32_t>();

                    for (uint32_t i = 0; i < count; i++)
                    {
                        Math::Vec3f pos;

                        pos.x = index % (uint32_t)mesh->GetSize().x;
                        pos.z = (uint32_t)(index / (uint32_t)mesh->GetSize().x);

                        if(m_Header.ZAxisOrientation == 0)
                            pos.y = ((uint32_t)mesh->GetSize().y - 1) - z;
                        else
                            pos.y = z;

                        index++;
                        cid = GetColorIdx(data);
                        if(cid == -1)
                            continue;

                        Beg = Beg.min(pos);
                        End = End.max(pos);
                        mesh->SetVoxel(pos, 0, cid, false);
                    }
                    
                }
                else
                {
                    Math::Vec3f pos;

                    pos.x = index % (uint32_t)mesh->GetSize().x;
                    pos.z = (uint32_t)(index / (uint32_t)mesh->GetSize().x);

                    if(m_Header.ZAxisOrientation == 0)
                        pos.y = ((uint32_t)mesh->GetSize().y - 1) - z;
                    else
                        pos.y = z;

                    index++;
                    cid = GetColorIdx(data);
                    if(cid == -1)
                        continue;

                    Beg = Beg.min(pos);
                    End = End.max(pos);
                    mesh->SetVoxel(pos, 0, cid, false);
                }
            }
        }
        mesh->BBox = CBBox(Beg, End);
    }

    int CQubicleBinaryFormat::GetColorIdx(int color)
    {
        int ret = 0;
        CColor c;
        if(m_Header.ColorFormat == 0)
            c.FromRGBA(color);
        else
            c.FromBGRA(color);

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
