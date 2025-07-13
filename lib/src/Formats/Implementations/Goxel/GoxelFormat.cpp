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

#include "GoxelFormat.hpp"
#include <cstring>
#include <VCore/Misc/Exceptions.hpp>
#include <VCore/Meshing/MaterialManager.hpp>

namespace VCore
{
    void CGoxelFormat::ParseFormat()
    {
        m_BeginX = INT32_MAX;
        m_EndX = 0;

        // Reads the whole file structure first.
        ReadFile();

        CGoxelModelParser parser(m_DataStream, m_BL16Offsets, m_Materials, m_Chunks, m_BeginX, m_EndX);

        VoxelModel m = std::make_shared<CVoxelSpace>();
        for (auto &&chunkData : m_Chunks)
        {
            auto chunkpos = GetChunkpos(chunkData.first);
            if constexpr(Config::ChunkSize == 8)
            {
                for (int x = 0; x < 16; x += 8)
                {
                    for (int y = 0; y < 16; y += 8)
                    {
                        for (int z = 0; z < 16; z += 8)
                        {
                            CreateChunk(chunkpos + Math::Vec3i(x, y, z), m, parser);
                        }
                    }
                }
            }
            else
                CreateChunk(chunkpos, m, parser);
        }
        
        if(m_Mode == FileMode::STREAMED)
            m->SetStream(new CGoxelStreamable(m_IOHandler, m_DataStream->GetFilePath(), std::move(m_BL16Offsets), std::move(m_Materials), std::move(m_Chunks), m_BeginX, m_EndX));

        m_Chunks.clear();
        m_Materials.clear();
    }

    void CGoxelFormat::CreateChunk(const Math::Vec3i &p_ChunkPos, VoxelModel &p_Model, CGoxelModelParser &p_Parser)
    {
        auto chunk = p_Model->GetChunk(p_ChunkPos);
        if(!chunk)
        {
            chunk = p_Model->CreateOrGetChunk(p_ChunkPos);
            if(m_Mode != FileMode::STREAMED)
                p_Parser.FillChunk(p_ChunkPos, chunk);
        }
    }

    void CGoxelFormat::ReadFile()
    {
        std::string signature(4, '\0');
        m_DataStream->Read(&signature[0], 4);
        signature += "\0";

        // Checks the file header
        if(signature != "GOX ")
            throw CVoxelFormatException("Unknown file format");

        int version = m_DataStream->Read<int>();
        if(version != 2)
            throw CVoxelFormatException("Version: " + std::to_string(version) + " is not supported");

        while (!m_DataStream->Eof())
        {
            SGoxelChunkHeader chunk = m_DataStream->Read<SGoxelChunkHeader>();

            if(strncmp(chunk.Type, "BL16", sizeof(chunk.Type)) == 0)
                ProcessBL16(chunk);
            else if(strncmp(chunk.Type, "LAYR", sizeof(chunk.Type)) == 0)
                ProcessLayer(chunk);
            else if(strncmp(chunk.Type, "MATE", sizeof(chunk.Type)) == 0)
                ProcessMaterial(chunk);
            else
                m_DataStream->Seek(chunk.Size + sizeof(int));
        }
    }

    void CGoxelFormat::ProcessMaterial(const SGoxelChunkHeader &p_Chunk)
    {
        auto dict = ReadDict(p_Chunk, m_DataStream->Tell());

        float c[4];
        memcpy(c, dict["color"].data(), 4 * sizeof(float));

        CMaterial mat;
        mat.Transparency = 1.f - c[3];
        mat.Metallic = *((float*)(dict["metallic"]).data());
        mat.Roughness = *((float*)(dict["roughness"]).data());
        mat.Emission = *((float*)(dict["emission"]).data());

        m_Materials.push_back(mat);
        m_DataStream->Seek(sizeof(int));
    }

    void CGoxelFormat::ProcessLayer(const SGoxelChunkHeader &p_Chunk)
    {
        auto startPos = m_DataStream->Tell();
        uint32_t blocks = m_DataStream->Read<uint32_t>();

        // Skip to the metadata of the layer.
        m_DataStream->Seek(blocks * sizeof(int) * 5);
        auto dict = ReadDict(p_Chunk, startPos);
        auto material = *((uint32_t*)(dict["material"].data()));
        auto visible = *((int*)(dict["visible"].data()));
        if(!visible)
        {
            m_DataStream->Seek(sizeof(int));
            return;
        }

        // Go back to the beginning of the layer.
        m_DataStream->Seek(startPos + sizeof(uint32_t), VCore::SeekOrigin::BEG);

        // Get all chunks
        for (uint32_t i = 0; i < blocks; i++)
        {
            auto bl16Index = m_DataStream->Read<uint32_t>();
            Math::Vec3i position;

            position.x = m_DataStream->Read<int>();
            position.z = m_DataStream->Read<int>();
            position.y = m_DataStream->Read<int>();

            // Goxel uses z as up axis. We use y.
            m_BeginX = std::min(m_BeginX, position.x);
            m_EndX = std::max(m_EndX, position.x + 16);

            m_DataStream->Seek(sizeof(int));

            m_Chunks[position].push_back(ChunkInfo {bl16Index, static_cast<uint8_t>(material)});
        }
        m_DataStream->Seek(startPos + p_Chunk.Size + sizeof(int), VCore::SeekOrigin::BEG);
    }

    void CGoxelFormat::ProcessBL16(const SGoxelChunkHeader &p_Chunk)
    {
        m_BL16Offsets.push_back(m_DataStream->Tell() - sizeof(p_Chunk));
        m_DataStream->Seek(p_Chunk.Size + sizeof(int));
    }

    ankerl::unordered_dense::map<std::string, std::string> CGoxelFormat::ReadDict(const SGoxelChunkHeader &p_Chunk, size_t const p_StartPos)
    {
        ankerl::unordered_dense::map<std::string, std::string> ret;

        while (m_DataStream->Tell() - p_StartPos < (size_t)p_Chunk.Size)
        {
            auto size = m_DataStream->Read<uint32_t>();
            std::string key(size, '\0');
            m_DataStream->Read(&key[0], size);

            size = m_DataStream->Read<uint32_t>();
            std::string value(size, '\0');
            m_DataStream->Read(&value[0], size);

            ret[key] = value;
        }

        return ret;
    }
}
