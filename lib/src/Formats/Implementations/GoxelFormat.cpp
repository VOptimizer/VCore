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

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "GoxelFormat.hpp"
#include <string.h>
#include <VCore/Misc/Exceptions.hpp>

namespace VCore
{
    void CGoxelFormat::ParseFormat()
    {
        m_HasEmission = false;

        ReadFile();
        
        std::map<int, int> ColorIdx;
        std::map<int, int> EmissionColorIdx;
        
        for (auto &&l : m_Layers)
        {
            // Ignore invisible layers.
            if(!l.Visible)
                continue;

            Math::Vec3f Beg(INFINITY, INFINITY, INFINITY), End, TranslationBeg(INFINITY, INFINITY, INFINITY);
            VoxelModel m = std::make_shared<CVoxelModel>();
            m->Name = l.Name;
            m->SetSize(m_BBox.End + m_BBox.Beg.abs());
            std::map<int, int> meshMaterialMapping;

            for (auto &&b : l.Blocks)
            {
                Math::Vec3f v = b.Pos;
                BL16 &tmp = m_BL16s[b.Index];

                for (int z = v.z; z < v.z + 16; z++)
                {
                    for (int y = v.y; y < v.y + 16; y++)
                    {
                        for (int x = v.x; x < v.x + 16; x++)
                        {
                            // Makes y the up axis, as needed.
                            // Also Goxel uses a left handed coordinate system, VCore uses a right handed one. So we need to convert the coordinates.
                            Math::Vec3i vi(m->GetSize().x - x, z, y);
                            vi += m_BBox.Beg.abs();
                            // vi *= Math::Vec3i(1, 1, -1);

                            //TODO: Handle negative pos.
                            uint32_t p = tmp.GetVoxel(Math::Vec3i(x - v.x, y - v.y, z - v.z));
                            if((p & 0xFF000000) != 0)
                            {
                                int IdxC = 0;
                                bool found = false;
                                int matIdx = 0;

                                auto it = meshMaterialMapping.find(l.MatIdx);
                                if(it != meshMaterialMapping.end())
                                    matIdx = it->second;
                                else
                                {
                                    auto material = m_Materials[l.MatIdx];
                                    m->Materials.push_back(material);
                                    matIdx = m->Materials.size() - 1;
                                    meshMaterialMapping[l.MatIdx] = matIdx;
                                }

                                if(m_HasEmission)
                                {
                                    auto texIT = m_Textures.find(TextureType::EMISSION);
                                    if(texIT == m_Textures.end())
                                        m_Textures[TextureType::EMISSION] = std::make_shared<CTexture>();

                                    auto material = m_Materials[l.MatIdx];
                                    if(material->Power > 0)
                                    {
                                        if(EmissionColorIdx.find(p) == EmissionColorIdx.end())
                                        {
                                            CColor c;
                                            memcpy(c.c, &p, 4);

                                            m_Textures[TextureType::EMISSION]->AddPixel(c);
                                            IdxC = m_Textures[TextureType::EMISSION]->GetSize().x - 1;
                                            EmissionColorIdx[p] = IdxC;
                                        }
                                        else
                                            IdxC = EmissionColorIdx[p];

                                        found = true;
                                    }
                                }

                                if(!found)
                                {
                                    if(ColorIdx.find(p) == ColorIdx.end())
                                    {
                                        CColor c;
                                        memcpy(c.c, &p, 4);

                                        auto texIT = m_Textures.find(TextureType::DIFFIUSE);
                                        if(texIT == m_Textures.end())
                                            m_Textures[TextureType::DIFFIUSE] = std::make_shared<CTexture>();

                                        m_Textures[TextureType::DIFFIUSE]->AddPixel(c);

                                        if(m_HasEmission)
                                            m_Textures[TextureType::EMISSION]->AddPixel(CColor(0, 0, 0, 255));
                                        
                                        IdxC = m_Textures[TextureType::DIFFIUSE]->GetSize().x - 1;
                                        ColorIdx[p] = IdxC;
                                    }
                                    else
                                        IdxC = ColorIdx[p];
                                }
                                
                                TranslationBeg = TranslationBeg.min(Math::Vec3f(x, y, z));
                                Beg = Beg.min(vi);
                                End = End.max(vi);

                                m->SetVoxel(vi, matIdx, IdxC, false);
                            }
                        }
                    }
                }
            }
            
            m->BBox = CBBox(Beg, End);
            m->GenerateVisibilityMask();

            // TODO: This is dumb! The model matrix should be created on a central point!
            auto sceneNode = std::make_shared<CSceneNode>();
            m->Pivot = m->BBox.GetSize() / 2;
            Math::Vec3f translation = TranslationBeg + m->Pivot;
            std::swap(translation.y, translation.z);
            translation.z *= -1;

            sceneNode->Mesh = m;
            sceneNode->Position = translation;
            m_SceneTree->AddChild(sceneNode);
            m_SceneTree->Visible = l.Visible;

            m_Models.push_back(m);
        }

        for (auto &&m : m_Models)
            m->Textures = m_Textures;
        
        m_BBox = CBBox();
        m_BL16s.clear();
        m_Layers.clear();
    }

    void CGoxelFormat::ReadFile()
    {
        std::string Signature(4, '\0');
        m_DataStream->Read(&Signature[0], 4);
        Signature += "\0";

        // Checks the file header
        if(Signature != "GOX ")
            throw CVoxelLoaderException("Unknown file format");

        int Version = m_DataStream->Read<int>();
        if(Version != 2)
            throw CVoxelLoaderException("Version: " + std::to_string(Version) + " is not supported");

        while (!m_DataStream->Eof())
        {
            SChunkHeader Chunk = m_DataStream->Read<SChunkHeader>();

            if(strncmp(Chunk.Type, "BL16", sizeof(Chunk.Type)) == 0)
                ProcessBL16(Chunk);
            else if(strncmp(Chunk.Type, "LAYR", sizeof(Chunk.Type)) == 0)
                ProcessLayer(Chunk);
            else if(strncmp(Chunk.Type, "MATE", sizeof(Chunk.Type)) == 0)
                ProcessMaterial(Chunk);
            else
                m_DataStream->Seek(Chunk.Size + sizeof(int));
        }
    }

    void CGoxelFormat::ProcessMaterial(const SChunkHeader &Chunk)
    {
        auto Dict = ReadDict(Chunk, m_DataStream->Tell());
        m_Materials.push_back(std::make_shared<CMaterial>());

        float c[4];
        memcpy(c, Dict["color"].data(), 4 * sizeof(float));

        m_Materials.back()->Transparency = 1.f - c[3];
        m_Materials.back()->Metallic = *((float*)(Dict["metallic"]).data());
        m_Materials.back()->Roughness = *((float*)(Dict["roughness"]).data());
        m_Materials.back()->Power = *((float*)(Dict["emission"]).data());

        if(m_Materials.back()->Power != 0.0)
            m_HasEmission = true;

        m_DataStream->Seek(sizeof(int));
    }

    void CGoxelFormat::ProcessLayer(const SChunkHeader &Chunk)
    {
        Layer l;
        size_t StartPos = m_DataStream->Tell();

        int Blocks = m_DataStream->Read<int>();

        for (int i = 0; i < Blocks; i++)
        {
            Block B;

            B.Index = m_DataStream->Read<int>();
            B.Pos.x = m_DataStream->Read<int>();
            B.Pos.y = m_DataStream->Read<int>();
            B.Pos.z = m_DataStream->Read<int>();

            Math::Vec3i v = B.Pos + Math::Vec3f(16, 16, 16);

            // Goxel uses z as up axis. We use y.
            m_BBox.Beg = m_BBox.Beg.min(Math::Vec3i(B.Pos.x, B.Pos.z, B.Pos.y));
            m_BBox.End = m_BBox.End.max(Math::Vec3i(v.x, v.z, v.y));

            m_DataStream->Seek(sizeof(int));

            l.Blocks.push_back(B);
        }

        auto Dict = ReadDict(Chunk, StartPos);
        l.MatIdx = *((int*)(Dict["material"].data()));
        l.Name = Dict["name"];
        l.Visible = *((int*)(Dict["visible"].data()));
        
        m_Layers.push_back(l);
        m_DataStream->Seek(sizeof(int));
    }

    void CGoxelFormat::ProcessBL16(const SChunkHeader &Chunk)
    {
        stbi_uc *PngData = new stbi_uc[Chunk.Size];
        m_DataStream->Read((char*)PngData, Chunk.Size);

        int w, h, c;
        uint32_t *ImgData = (uint32_t*)stbi_load_from_memory(PngData, Chunk.Size, &w, &h, &c, 4);
        m_BL16s.emplace_back();
        m_BL16s.back().SetData(ImgData);
        delete[] ImgData;
        delete[] PngData;

        m_DataStream->Seek(sizeof(int));
    }

    std::map<std::string, std::string> CGoxelFormat::ReadDict(const SChunkHeader &Chunk, size_t StartPos)
    {
        std::map<std::string, std::string> Ret;

        while (m_DataStream->Tell() - StartPos < (size_t)Chunk.Size)
        {
            int Size = m_DataStream->Read<int>();
            std::string Key(Size, '\0');
            m_DataStream->Read(&Key[0], Size);
            // Key += '\0';

            Size = m_DataStream->Read<int>();
            std::string Value(Size, '\0');
            m_DataStream->Read(&Value[0], Size);

            Ret[Key] = Value;
        }

        return Ret;
    }
}
