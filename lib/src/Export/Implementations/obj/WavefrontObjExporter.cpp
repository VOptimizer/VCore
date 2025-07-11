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

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <VCore/Meshing/MaterialManager.hpp>
#include "WavefrontObjExporter.hpp"
#include "../../../FileUtils.hpp"
#include "VCore/Math/Vector.hpp"
#include "VCore/Meshing/Color.hpp"
#include "VCore/Meshing/Texture.hpp"
#include "VCore/Misc/FileStream.hpp"
#include "VCore/Misc/unordered_dense.h"
#include <format>
#include <memory>
#include <string>
#include <cmath>

namespace VCore
{   
    void CWavefrontObjExporter::WriteHeaderData(const fast_vector<Mesh> &)
    {
        m_FilenameWithoutExt = GetFilenameWithoutExt(m_Path);
        std::string filePathWithoutExt = GetPathWithoutExt(m_Path);

        m_ObjFile = m_IOHandler->Open(filePathWithoutExt + ".obj", "w+b");
        m_MtlFile = m_IOHandler->Open(filePathWithoutExt + ".mtl", "wb");

        m_ObjFile->Write(std::format("# {}\n", IExporter::WATERMARK));
        m_ObjFile->Write("mtllib " + m_FilenameWithoutExt + ".mtl\n");
    }

    void CWavefrontObjExporter::WriteMeshData(const Mesh &p_Mesh)
    {
        // Name of the mesh
        m_ObjFile->Write("o " + GetMeshName(p_Mesh) + "\n");

        //"Extracts" the rotation matrix. Quick'n Dirty
        Math::Mat4x4 rotMat = p_Mesh->ModelMatrix;
        rotMat.x.w = 0;
        rotMat.y.w = 0;
        rotMat.z.w = 0;

        for (auto &&surface : p_Mesh->Surfaces)
        {
            for(uint64_t i = 0; i < surface->GetVertexCount(); i++)
            {
                auto vertex = surface->GetVertex(i);
                Math::Vec3f pos = vertex.Pos;
                Math::Vec3f normal = vertex.Normal;

                if(Settings->WorldSpace)
                {
                    pos = p_Mesh->ModelMatrix * pos;
                    normal = rotMat * normal;
                }

                m_ObjFile->Write(std::format("v {} {} {}\n", pos.x, pos.y, pos.z));
                m_ObjFile->Write(std::format("vn {} {} {}\n", normal.x, normal.y, normal.z));
                m_ObjFile->Write(std::format("vt {:<#10x}{:3}\n", vertex.Color, "")); // Placeholder uv will be patched later.

                m_Colors.insert(vertex.Color);
            }

            auto material = GetObjMaterial(surface->MaterialHandle);

            m_ObjFile->Write("usemtl " + material + "\n");

            // Writes the indices
            for (uint64_t i = 0; i < surface->GetFaceCount(); i++)
            {
                m_ObjFile->Write("f");
                for (char j = 0; j < 3; j++)
                {
                    int index = surface->GetIndex(i * 3 + j) + m_IndexOffset + 1;
                    m_ObjFile->Write(std::format(" {0}/{0}/{0}", index));
                }
                m_ObjFile->Write("\n");
            }
            m_IndexOffset += surface->GetVertexCount();
        }
    }

    void CWavefrontObjExporter::WriteFooterData()
    {
        GenerateTextureAndPatchUV();

        m_Colors.clear();
        m_Materials.clear();
        m_FilenameWithoutExt.clear();
        m_IndexOffset = 0;

        m_IOHandler->Close(m_ObjFile);
        m_IOHandler->Close(m_MtlFile);
    }

    std::string CWavefrontObjExporter::GetObjMaterial(const uint8_t p_MaterialHandle)
    {
        auto it = m_Materials.find(p_MaterialHandle);
        if(it == m_Materials.end())
        {
            auto material = MaterialManager::GetMaterial(p_MaterialHandle);
            if(!material)
                material = MaterialManager::GetMaterial(0);

            float ambient = 1.0;
            int illum = 2;
            float transparency = 0;
            float alpha = 1.0;

            if(material->Metallic != 0.0)
            {
                ambient = material->Metallic;
                illum = 3;
            }
            else if(material->Transparency != 0.0) // Glass
            {
                illum = 4;
                transparency = material->Transparency;
                alpha = 1 - material->Transparency;
            }

            m_MtlFile->Write(std::format("newmtl Mat{}\n", m_Materials.size()));
            m_MtlFile->Write(std::format("Ns {}\n", material->Roughness * 1000.f));
            m_MtlFile->Write(std::format("Ka {0} {0} {0}\n", ambient));
            m_MtlFile->Write("Kd 1.0 1.0 1.0\n");
            m_MtlFile->Write(std::format("Ks {0} {0} {0}\n", material->Specular));

            if(material->Emission != 0.0)
            {
                m_MtlFile->Write(std::format("Ke {0} {0} {0}\n", material->Emission));
                // m_MtlFile->Write("map_Ke " + filenameWithoutExt + ".emission.png\n");
            }

            m_MtlFile->Write(std::format("Tr {}\n", transparency));
            m_MtlFile->Write(std::format("d {}\n", alpha));
            m_MtlFile->Write(std::format("Ni {}\n", material->IOR));
            m_MtlFile->Write(std::format("illum {}\n", illum));
            m_MtlFile->Write(std::format("map_Kd {}.albedo.png\n", m_FilenameWithoutExt));

            it = m_Materials.insert({p_MaterialHandle, m_Materials.size()}).first;
        }

        return std::format("Mat{}", it->second);
    }

    void CWavefrontObjExporter::GenerateTextureAndPatchUV()
    {
        auto p2Size = GetTextureSizeP2();
        auto texture = std::make_shared<CTexture>(Math::Vec2ui(p2Size, p2Size));

        ankerl::unordered_dense::map<uint32_t, Math::Vec2f> uvMapping;
        uint64_t pixelIndex = 0;

        m_ObjFile->Seek(0, SeekOrigin::BEG);
        while (!m_ObjFile->Eof()) 
        {
            auto line = m_ObjFile->ReadLine();
            if(line.starts_with("vt"))
            {
                auto color = static_cast<uint32_t>(std::stoi(line.substr(3, line.find_first_of(" ", 3)), nullptr, 16)) | 0xFF000000;
                auto it = uvMapping.find(color);
                if(it == uvMapping.end())
                {
                    Math::Vec2ui position(pixelIndex % p2Size, pixelIndex / p2Size);
                    pixelIndex++;

                    texture->AddPixel(CColor(color), position);
                    it = uvMapping.insert({color, (Math::Vec2f(position.x, (p2Size - 1) - position.y) + Math::Vec2f(.5f, .5f)) / texture->GetSize()}).first;
                }

                // Patch the uv coordinates.
                m_ObjFile->Seek(m_ObjFile->Tell() - 1 - line.size(), SeekOrigin::BEG);
                m_ObjFile->Write(std::format("vt {:.4f} {:.4f}\n", it->second.x, it->second.y));
            }
        }

        SaveTexture(texture, m_FilenameWithoutExt, "albedo");
    }

    uint64_t CWavefrontObjExporter::GetTextureSizeP2()
    {
        auto size = std::ceil(std::sqrt(m_Colors.size()));
        return std::pow(2, std::ceil(std::log2(size)));
    }
}
