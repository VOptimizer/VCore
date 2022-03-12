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
#include <VoxelOptimizer/Misc/Exceptions.hpp>

namespace VoxelOptimizer
{
    void CGoxelFormat::ParseFormat()
    {
        m_HasEmission = false;

        ReadFile();
        
        std::map<int, int> ColorIdx;
        std::map<int, int> EmissionColorIdx;
        
        for (auto &&l : m_Layers)
        {
            CVector Beg(INFINITY, INFINITY, INFINITY), End, TranslationBeg(INFINITY, INFINITY, INFINITY);
            VoxelMesh m = VoxelMesh(new CVoxelMesh());
            m->SetName(l.Name);
            m->SetSize(m_BBox.End + m_BBox.Beg.Abs());
            std::map<int, int> meshMaterialMapping;

            for (auto &&b : l.Blocks)
            {
                CVector v = b.Pos;
                BL16 &tmp = m_BL16s[b.Index];

                for (int z = v.z; z < v.z + 16; z++)
                {
                    for (int y = v.y; y < v.y + 16; y++)
                    {
                        for (int x = v.x; x < v.x + 16; x++)
                        {
                            CVectori vi(x, y, z);
                            vi += m_BBox.Beg.Abs();

                            //TODO: Handle negative pos.
                            uint32_t p = tmp.GetVoxel(CVectori(x - v.x, y - v.y, z - v.z));
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
                                    m->Materials().push_back(material);
                                    matIdx = m->Materials().size() - 1;
                                    meshMaterialMapping[l.MatIdx] = matIdx;
                                }

                                if(m_HasEmission)
                                {
                                    auto texIT = m_Textures.find(TextureType::EMISSION);
                                    if(texIT == m_Textures.end())
                                        m_Textures[TextureType::EMISSION] = Texture(new CTexture());

                                    auto material = m_Materials[l.MatIdx];
                                    if(material->Power > 0)
                                    {
                                        if(EmissionColorIdx.find(p) == EmissionColorIdx.end())
                                        {
                                            CColor c;
                                            memcpy(c.c, &p, 4);

                                            m_Textures[TextureType::EMISSION]->AddPixel(c);
                                            IdxC = m_Textures[TextureType::EMISSION]->Size().x - 1;
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
                                            m_Textures[TextureType::DIFFIUSE] = Texture(new CTexture());

                                        m_Textures[TextureType::DIFFIUSE]->AddPixel(c);

                                        if(m_HasEmission)
                                            m_Textures[TextureType::EMISSION]->AddPixel(CColor(0, 0, 0, 255));
                                        
                                        IdxC = m_Textures[TextureType::DIFFIUSE]->Size().x - 1;
                                        ColorIdx[p] = IdxC;
                                    }
                                    else
                                        IdxC = ColorIdx[p];
                                }
                                
                                TranslationBeg = TranslationBeg.Min(CVector(x, y, z));
                                Beg = Beg.Min(vi);
                                End = End.Max(vi);

                                m->SetVoxel(vi, matIdx, IdxC, false);
                            }
                        }
                    }
                }
            }
            
            m->SetBBox(CBBox(Beg, End));

            // TODO: This is dumb! The model matrix should be created on a central point!
            auto sceneNode = SceneNode(new CSceneNode());
            sceneNode->SetLocalOffset(m->GetBBox().GetSize() / 2);
            CVector translation = TranslationBeg + sceneNode->GetLocalOffset();
            std::swap(translation.y, translation.z);
            translation.z *= -1;

            sceneNode->SetMesh(m);
            sceneNode->SetPosition(translation);
            m_SceneTree->AddChild(sceneNode);

            m_Models.push_back(m);
        }

        for (auto &&m : m_Models)
            m->Colorpalettes() = m_Textures;
        
        m_BBox = CBBox();
        m_BL16s.clear();
        m_Layers.clear();
    }

    void CGoxelFormat::ReadFile()
    {
        std::string Signature(4, '\0');
        ReadData(&Signature[0], 4);
        Signature += "\0";

        // Checks the file header
        if(Signature != "GOX ")
            throw CVoxelLoaderException("Unknown file format");

        int Version = ReadData<int>();
        if(Version != 2)
            throw CVoxelLoaderException("Version: " + std::to_string(Version) + " is not supported");

        while (!IsEof())
        {
            SChunkHeader Chunk = ReadData<SChunkHeader>();

            if(strncmp(Chunk.Type, "BL16", sizeof(Chunk.Type)) == 0)
                ProcessBL16(Chunk);
            else if(strncmp(Chunk.Type, "LAYR", sizeof(Chunk.Type)) == 0)
                ProcessLayer(Chunk);
            else if(strncmp(Chunk.Type, "MATE", sizeof(Chunk.Type)) == 0)
                ProcessMaterial(Chunk);
            else
                Skip(Chunk.Size + sizeof(int));
        }
    }

    void CGoxelFormat::ProcessMaterial(const SChunkHeader &Chunk)
    {
        auto Dict = ReadDict(Chunk, Tellg());
        m_Materials.push_back(Material(new CMaterial()));

        float c[4];
        memcpy(c, Dict["color"].data(), 4 * sizeof(float));

        m_Materials.back()->Transparency = 1.f - c[3];
        m_Materials.back()->Metallic = *((float*)(Dict["metallic"]).data());
        m_Materials.back()->Roughness = *((float*)(Dict["roughness"]).data());
        m_Materials.back()->Power = *((float*)(Dict["emission"]).data());

        if(m_Materials.back()->Power != 0.0)
            m_HasEmission = true;

        Skip(sizeof(int));
    }

    void CGoxelFormat::ProcessLayer(const SChunkHeader &Chunk)
    {
        Layer l;
        size_t StartPos = Tellg();

        int Blocks = ReadData<int>();

        for (size_t i = 0; i < Blocks; i++)
        {
            Block B;

            B.Index = ReadData<int>();
            B.Pos.x = ReadData<int>();
            B.Pos.y = ReadData<int>();
            B.Pos.z = ReadData<int>();

            CVector v = B.Pos + CVector(16, 16, 16);

            m_BBox.Beg = m_BBox.Beg.Min(B.Pos);
            m_BBox.End = m_BBox.End.Max(v);

            Skip(sizeof(int));

            l.Blocks.push_back(B);
        }

        auto Dict = ReadDict(Chunk, StartPos);
        l.MatIdx = *((int*)(Dict["material"].data()));
        l.Name = Dict["name"];
        
        m_Layers.push_back(l);
        Skip(sizeof(int));
    }

    void CGoxelFormat::ProcessBL16(const SChunkHeader &Chunk)
    {
        stbi_uc *PngData = new stbi_uc[Chunk.Size];
        ReadData((char*)PngData, Chunk.Size);

        int w, h, c;
        uint32_t *ImgData = (uint32_t*)stbi_load_from_memory(PngData, Chunk.Size, &w, &h, &c, 4);
        m_BL16s.emplace_back();
        m_BL16s.back().SetData(ImgData);
        delete[] ImgData;
        delete[] PngData;

        Skip(sizeof(int));
    }

    std::map<std::string, std::string> CGoxelFormat::ReadDict(const SChunkHeader &Chunk, size_t StartPos)
    {
        std::map<std::string, std::string> Ret;

        while (Tellg() - StartPos < Chunk.Size)
        {
            int Size = ReadData<int>();
            std::string Key(Size, '\0');
            ReadData(&Key[0], Size);
            // Key += '\0';

            Size = ReadData<int>();
            std::string Value(Size, '\0');
            ReadData(&Value[0], Size);

            Ret[Key] = Value;
        }

        return Ret;
    }
} // namespace VoxelOptimizer
