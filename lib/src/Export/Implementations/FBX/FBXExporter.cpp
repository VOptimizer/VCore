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

#include <VoxelOptimizer/Misc/Exceptions.hpp>
#include "FBXExporter.hpp"
#include "FBXNodes.hpp"

namespace VoxelOptimizer
{
    const static char HEADER[] = "Kaydara FBX Binary\x20\x20\x00\x1A\x00";
    const static char *CREATIONTIME = "1970-01-01 10:00:00:000";
    const static char FILEID[] = "\x28\xb3\x2a\xeb\xb6\x24\xcc\xc2\xbf\xc8\xb0\x2a\xa9\x2b\xfc\xf1";
    const static int FBX_VERSION = 7400;

    std::map<std::string, std::vector<char>> CFBXExporter::Generate(std::vector<Mesh> Meshes)
    {
        WriteHeader();
        WriteHeaderExtension();
        
        auto node = std::make_shared<CGlobalSettings>();
        node->Serialize(m_Stream);

        node = std::make_shared<CDocuments>();
        node->Serialize(m_Stream);

        node = std::make_shared<CFBXNode>();
        node->SetName("References");
        node->Serialize(m_Stream);



        std::vector<char> fbx = m_Stream.data();

        // TODO: Clear Stream

        return {{"fbx", fbx}};
    }

    void CFBXExporter::WriteHeader()
    {
        // Kaydara FBX header
        m_Stream.write(HEADER, sizeof(HEADER) - 1);

        // FBX version
        m_Stream.write((char*)&FBX_VERSION, sizeof(FBX_VERSION));
    }

    void CFBXExporter::WriteHeaderExtension()
    {
        auto node = std::make_shared<CFBXHeaderExtension>();
        node->Serialize(m_Stream);

        node = CFBXNode::CreateNestedNode("FileId");
        node->AddProperty(std::make_shared<CFBXProperty<const char*>>(FILEID, sizeof(FILEID) - 1));
        node->Serialize(m_Stream);

        node = CFBXNode::CreateNestedNode("CreationTime");
        node->AddProperty(std::make_shared<CFBXProperty<std::string>>(CREATIONTIME)));
        node->Serialize(m_Stream);

        node = CFBXNode::CreateNestedNode("Creator");
        node->AddProperty(std::make_shared<CFBXProperty<std::string>>("Created with VoxelOptimzer"));
        node->Serialize(m_Stream);
    }

    void CFBXExporter::WriteDefinitions()
    {
        CBinaryStream strm;
        int totalCount = 1;

        auto node = std::make_shared<CFBXNode>();
        node->SetName("References");
        auto nested = node->CreateNestedNode("GlobalSettings");
        auto count = nested->CreateNestedNode("Count");
        count->AddProperty(std::make_shared<CFBXProperty<int>>(1));
        nested->AddNestedNode(count);
        node->AddNestedNode(nested);
        node->Serialize(strm);
    }
} // namespace VoxelOptimizer