/*
 * MIT License
 *
 * Copyright (c) 2022 Christian Tost
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
#include <stdexcept>
#include <VoxelOptimizer/Misc/Exceptions.hpp>
#include <VoxelOptimizer/Formats/IVoxelFormat.hpp>

#include "../FileUtils.hpp"

#include "Implementations/MagicaVoxelFormat.hpp"
#include "Implementations/KenshapeFormat.hpp"
#include "Implementations/GoxelFormat.hpp"
#include "Implementations/Qubicle/QubicleBinaryFormat.hpp"
#include "Implementations/Qubicle/QubicleBinaryTreeFormat.hpp"
#include "Implementations/Qubicle/QubicleExchangeFormat.hpp"
#include "Implementations/Qubicle/QubicleFormat.hpp"

namespace VoxelOptimizer
{
    VoxelFormat IVoxelFormat::Create(LoaderType type)
    {
        switch (type)
        {
            case LoaderType::MAGICAVOXEL: return VoxelFormat(new CMagicaVoxelFormat());
            case LoaderType::GOXEL: return VoxelFormat(new CGoxelFormat());
            case LoaderType::KENSHAPE: return VoxelFormat(new CKenshapeFormat());
            case LoaderType::QUBICLE_BIN: return VoxelFormat(new CQubicleBinaryFormat());
            case LoaderType::QUBICLE_BIN_TREE: return VoxelFormat(new CQubicleBinaryTreeFormat());
            case LoaderType::QUBICLE_EXCHANGE: return VoxelFormat(new CQubicleExchangeFormat());
            case LoaderType::QUBICLE: return VoxelFormat(new CQubicleFormat());

            default: throw CVoxelLoaderException("Unknown file type!");
        }
    }

    void IVoxelFormat::Save(const std::string &path, const std::vector<VoxelMesh> &meshes)
    {
        std::ofstream out(path, std::ios::binary);
        if(!out.is_open())
            throw CVoxelLoaderException("Failed to open '" + path + "'");

        try
        {
            auto File = Save(meshes);
            out.write(File.data(), File.size());
        }
        catch(const std::exception& e)
        {
            out.close();
            throw;
        }
        
        out.close();
    }

    std::vector<char> IVoxelFormat::Save(const std::vector<VoxelMesh> &meshes)
    {
        throw std::runtime_error("IVoxelFormat::Save is not implemented!");
    }

    void IVoxelFormat::ClearCache()
    {
        m_Models.clear();
        m_Materials.clear();
        m_Textures.clear();

        m_SceneTree = std::make_shared<CSceneNode>();
    }

    VoxelFormat IVoxelFormat::CreateAndLoad(const std::string &filename)
    {
        auto loader = Create(GetType(filename));
        loader->Load(filename);

        return loader;
    }

    LoaderType IVoxelFormat::GetType(const std::string &filename)
    {
        std::string ext = GetFileExt(filename);
        LoaderType type = LoaderType::UNKNOWN;
        
        if(ext == "vox")
            type = LoaderType::MAGICAVOXEL;
        else if(ext == "gox")
            type = LoaderType::GOXEL;
        else if(ext == "kenshape")
            type = LoaderType::KENSHAPE;
        else if(ext == "qb")
            type = LoaderType::QUBICLE_BIN;
        else if(ext == "qbt")
            type = LoaderType::QUBICLE_BIN_TREE;
        else if(ext == "qef")
            type = LoaderType::QUBICLE_EXCHANGE;
        else if(ext == "qbcl")
            type = LoaderType::QUBICLE;

        return type;
    }

    void IVoxelFormat::Load(const std::string &File)
    {
        std::ifstream in(File, std::ios::binary);
        if(in.is_open())
        {
            m_DataStream = in;
            ClearCache();
            ParseFormat();
        }
        else
            throw CVoxelLoaderException("Failed to open '" + File + "'");
    }

    void IVoxelFormat::Load(const char *Data, size_t Length)
    {
        m_DataStream = CBinaryStream(Data, Length);

        ClearCache();
        ParseFormat();
    }

    void IVoxelFormat::Combine(std::map<TextureType, Texture> &textures, std::vector<Material> &materials, const std::vector<VoxelMesh> &meshes)
    {
        if(meshes.empty() || textures.empty())
            return;

        std::map<int, int> colorMapping;
        auto otherTextures = meshes.front()->Colorpalettes;
        bool hasEmission = otherTextures.find(TextureType::EMISSION) != otherTextures.end();
        bool texturesHasEmission = textures.find(TextureType::EMISSION) != textures.end();

        // Creates a new emission texture
        if(hasEmission && !texturesHasEmission)
            textures[TextureType::EMISSION] = std::make_shared<CTexture>(textures[TextureType::DIFFIUSE]->GetSize());

        auto diffiuse = textures[TextureType::DIFFIUSE];
        auto otherDiffiuse = otherTextures[TextureType::DIFFIUSE];

        // Merge the textures
        for (int y = 0; y < otherDiffiuse->GetSize().y; y++)
        {
            for (int x = 0; x < otherDiffiuse->GetSize().x; x++)
            {
                int oldIdx = x + otherDiffiuse->GetSize().x * y;
                int newIdx = -1;

                auto &pixels = diffiuse->GetPixels();
                auto pixel = otherDiffiuse->GetPixel(Math::Vec2ui(x, y));

                auto it = std::find(pixels.begin(), pixels.end(), pixel);

                // Color not found
                if(it == pixels.end())
                {
                    // Add the new diffuse color
                    CColor c;
                    c.FromRGBA(pixel);
                    diffiuse->AddPixel(c);
                    
                    if (hasEmission)
                    {
                        // Add the emission color
                        auto emission = otherTextures[TextureType::EMISSION]->GetPixel(Math::Vec2ui(x, y));
                        c.FromRGBA(emission);

                        textures[TextureType::EMISSION]->AddPixel(c);
                    }
                    else if(texturesHasEmission)
                    {
                        c = CColor(0, 0, 0, 255);
                        textures[TextureType::EMISSION]->AddPixel(c);
                    }

                    newIdx = diffiuse->GetSize().x - 1;
                }
                else
                {
                    if (hasEmission)
                    {
                        uint32_t emission = 0, emission2 = -1;

                        // Check if the found color is an emission color.
                        while (emission != emission2)
                        {
                            emission = otherTextures[TextureType::EMISSION]->GetPixel(Math::Vec2ui(x, y));
                            emission2 = textures[TextureType::EMISSION]->GetPixel(Math::Vec2ui(x, y));
                            
                            if(emission != emission2)
                            {
                                it = std::find(it + 1, pixels.end(), pixel);
                                if(it == pixels.end())
                                    break;
                            }
                        }

                        // Add new color and emission if not found.
                        if(it == pixels.end())
                        {
                            CColor c;
                            c.FromRGBA(pixel);
                            diffiuse->AddPixel(c);
                            c.FromRGBA(emission);
                            textures[TextureType::EMISSION]->AddPixel(c);

                            newIdx = diffiuse->GetSize().x - 1;
                        }
                        else
                            newIdx = it - pixels.begin();
                    }
                    else
                        newIdx = it - pixels.begin();
                }

                colorMapping[oldIdx] = newIdx;
            }
        }
    
        for (auto &&m : meshes)
        {
            // Merges the materials
            for (auto &&mat : m->Materials)
            {
                auto it = std::find_if(materials.begin(), materials.end(), [mat](Material mat2)
                {
                    if(*mat == *mat2)
                        return true;

                    return false;
                });

                if(it != materials.end())
                    mat = (*it);
                else
                    materials.push_back(mat);
            }

            // Reassign the colors.
            for (auto &&v : m->GetVoxels())
                v.second->Color = colorMapping[v.second->Color];

            m->Colorpalettes = textures;
        }
        
    }

    void IVoxelFormat::ReadData(char *Buf, size_t Size)
    {
        size_t off = m_DataStream.offset();
        size_t size = m_DataStream.size();

        if(off + Size > size)
            throw CVoxelLoaderException("Unexpected file ending.");

        m_DataStream.read(Buf, Size);
    }

    bool IVoxelFormat::IsEof()
    {
        return m_DataStream.offset() >= m_DataStream.size();
    }

    size_t IVoxelFormat::Tellg()
    {
        return m_DataStream.offset();
    }

    void IVoxelFormat::Skip(size_t Bytes)
    {
        m_DataStream.skip(Bytes);
    }

    void IVoxelFormat::Reset()
    {
        m_DataStream.reset();
    }

    std::vector<char> IVoxelFormat::ReadDataChunk(size_t size)
    {
        std::vector<char> data(size, 0);
        ReadData(&data[0], data.size());

        return data;
    }
}
