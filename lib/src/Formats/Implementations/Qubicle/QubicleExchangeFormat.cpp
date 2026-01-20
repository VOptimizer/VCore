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

#include <VCore/Misc/Exceptions.hpp>
#include <VCore/Meshing/MaterialManager.hpp>
#include <cstdint>
#include <string>
#include "QubicleExchangeFormat.hpp"
#include "VCore/Formats/SceneNode.hpp"
#include "VCore/Meshing/Color.hpp"
#include "src/Misc/StringUtils.hpp"

namespace VCore
{
    void CQubicleExchangeFormat::ParseFormat()
    {
        if(ReadLine() != "Qubicle Exchange Format")
            throw CVoxelFormatException("Unknown file format");

        if(ReadLine() != "Version 0.2")
            throw CVoxelFormatException("Unsupported version!");

        ReadLine();

        VoxelModel mesh = std::make_shared<CVoxelSpace>();
        ParseVector<int>(ReadLine());
        ReadColors();
        ReadVoxels(mesh);

        auto sceneNode = new CSceneModelNode(nullptr, 0);
        SceneTree->AddChild(sceneNode);
        SceneTree->AddModel(mesh);
        m_Colors.clear();
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

    void CQubicleExchangeFormat::ReadColors()
    {
        int count = std::stoi(ReadLine());
        for (int i = 0; i < count; i++)
        {
            auto c = ParseVector<float>(ReadLine());
            m_Colors.push_back(CColor(c.x * 255.0, c.y * 255.0, c.z * 255.0, 255.0).AsRGBA());
        }
    }

    void CQubicleExchangeFormat::ReadVoxels(VoxelModel p_Mesh)
    {
        while (!m_DataStream->Eof())
        {
            auto contents = Split(ReadLine(), ' ', 2);
            if(contents.size() != 3)
                continue;

            Math::Vec3i pos = ParseVector<int>(contents[0]);

            uint32_t mask = std::stoi(contents[2]), cid = std::stoi(contents[1]);
            if(mask == 0)
                continue;

            uint32_t c = 0;
            if(cid < m_Colors.size())
                c = m_Colors[cid];

            p_Mesh->Insert({pos, CVoxel(c, 0)});
        }
    }
}
