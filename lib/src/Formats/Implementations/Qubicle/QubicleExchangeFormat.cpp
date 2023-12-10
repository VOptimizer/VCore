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

#include <sstream>
#include <VCore/Misc/Exceptions.hpp>
#include "QubicleExchangeFormat.hpp"

namespace VCore
{
    void CQubicleExchangeFormat::ParseFormat()
    {
        if(ReadLine() != "Qubicle Exchange Format")
            throw CVoxelLoaderException("Unknown file format");

        if(ReadLine() != "Version 0.2")
            throw CVoxelLoaderException("Unsupported version!");

        ReadLine();

        VoxelModel mesh = std::make_shared<CVoxelModel>();
        m_Materials.push_back(std::make_shared<CMaterial>());
        mesh->Materials = m_Materials;
        mesh->SetSize(ReadVector());
        ReadColors();
        ReadVoxels(mesh);

        auto sceneNode = std::make_shared<CSceneNode>();
        sceneNode->Mesh = mesh;
        m_SceneTree->AddChild(sceneNode);

        mesh->Textures = m_Textures;
        mesh->GenerateVisibilityMask();
        m_Models.push_back(mesh);
    }

    std::string CQubicleExchangeFormat::ReadLine()
    {
        std::string ret;

        char c = 0;
        do
        {
            c = m_DataStream->Read<char>();

            if(c != '\n' && c != '\r')
                ret += c;
        }while(c != '\n' && !m_DataStream->Eof());

        return ret;
    }

    Math::Vec3i CQubicleExchangeFormat::ReadVector()
    {
        std::stringstream strm;
        strm << ReadLine();

        Math::Vec3i ret;

        strm >> ret.x >> ret.y >> ret.z;

        return ret;
    }

    void CQubicleExchangeFormat::ReadColors()
    {
        std::stringstream strm;
        strm << ReadLine();

        int count;
        strm >> count;
        strm.clear();

        for (int i = 0; i < count; i++)
        {
            strm << ReadLine();
            float r, g, b;

            strm >> r >> g >> b;
            strm.clear();

            auto texIT = m_Textures.find(TextureType::DIFFIUSE);
            if(texIT == m_Textures.end())
                m_Textures[TextureType::DIFFIUSE] = std::make_shared<CTexture>();

            m_Textures[TextureType::DIFFIUSE]->AddPixel(CColor(r * 255.0, g * 255.0, b * 255.0, 255.0));
        }
    }

    void CQubicleExchangeFormat::ReadVoxels(VoxelModel mesh)
    {
        Math::Vec3i Beg(1000, 1000, 1000), End;
        while (!m_DataStream->Eof())
        {
            std::stringstream strm;
            strm << ReadLine();

            int mask, cid;

            Math::Vec3i pos;
            strm >> pos.x >> pos.y >> pos.z >> cid >> mask;

            if(mask == 0)
                continue;

            Beg = Beg.min(pos);
            End = End.max(pos);
            mesh->SetVoxel(pos, 0, cid, false);
        }
        mesh->BBox = CBBox(Beg, End);
    }
}
