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

#include "Kenshape.hpp"
#include "KenshapeFormat.hpp"
#include <stb_image.h>
#include <VoxelOptimizer/Misc/Exceptions.hpp>

namespace VoxelOptimizer
{
    void CKenshapeFormat::ParseFormat()
    {
        // Quick'n dirty gzip check.
        if(ReadData<uint8_t>() != 0x1f || ReadData<uint8_t>() != 0x8b || ReadData<uint8_t>() != 8)
            throw CVoxelLoaderException("Invalid file format!");

        Skip(7);

        int OutSize = 0;
        int pos = m_DataStream.tellg();

        m_DataStream.seekg(0, std::ios::end);
        size_t datasize = m_DataStream.tellg();
        m_DataStream.seekg(pos, std::ios::beg);

        std::vector<char> data = ReadDataChunk(datasize - pos);
        char *Data = stbi_zlib_decode_noheader_malloc(data.data(), data.size(), &OutSize);

        CJSON json;
        Kenshape Content;

        try
        {
            Content = json.Deserialize<Kenshape>(std::string(Data, OutSize));
            free(Data);
        }
        catch(const std::exception& e)
        {
            free(Data);
            throw CVoxelLoaderException("Invalid file format!");
        }

        VoxelMesh m = std::make_shared<CVoxelMesh>();
        auto mat = std::make_shared<CMaterial>();
        m_Materials.push_back(mat);
        m->Materials.push_back(mat);
        m->SetSize(Content->Size);

        std::map<int, int> ColorIdx;
        Math::Vec3f Pos;
        Math::Vec3f Beg, End;

        Pos.y = Content->Size.y - 1;
        Pos.z = (int)(Content->Size.z / 2.f);

        for (auto &&tile : Content->Tiles)
        {
            if(tile->ColorIdx != -1)
            {
                int IdxC = 0;
                if(ColorIdx.find(tile->ColorIdx) == ColorIdx.end())
                {
                    auto texIT = m_Textures.find(TextureType::DIFFIUSE);
                    if(texIT == m_Textures.end())
                        m_Textures[TextureType::DIFFIUSE] = std::make_shared<CTexture>();

                    m_Textures[TextureType::DIFFIUSE]->AddPixel(Content->Colors[tile->ColorIdx]);
                    IdxC = m_Textures[TextureType::DIFFIUSE]->GetSize().x - 1;
                    ColorIdx[tile->ColorIdx] = IdxC;
                }
                else
                    IdxC = ColorIdx[tile->ColorIdx];

                int Blocks = tile->Depth - 1;
                for (size_t z = Pos.z - Blocks; z <= Pos.z + Blocks; z++)
                {
                    Math::Vec3f v(Pos.x, Pos.y, z);
                    if(Beg.IsZero())
                        Beg = v;

                    Beg = Beg.min(v);
                    End = End.max(v);

                    m->SetVoxel(v, 0, IdxC, false);
                }
            }

            Pos.y--;
            if(Pos.y < 0)
            {
                Pos.y = Content->Size.y - 1;;
                Pos.x++;
            }
        }

        auto sceneNode = std::make_shared<CSceneNode>();
        m_SceneTree->AddChild(sceneNode);
        sceneNode->SetMesh(m);
        m->Pivot = m->GetSize() / 2;

        m->Textures = m_Textures;
        m->BBox = CBBox(Beg, End);
        m->GenerateVisibilityMask();

        m_Models.push_back(m);
    }
}
