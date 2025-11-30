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

#include <cmath>
#include <cstdint>
#include <cstring>
#include "GLTFExporter.hpp"
#include "Nodes.hpp"
#include "../../../FileUtils.hpp"
#include <VCore/Math/Vector.hpp>
#include <VCore/Misc/fast_vector.hpp>
#include <VCore/Misc/FileStream.hpp>
#include <VCore/Meshing/MaterialManager.hpp>
#include <utility>

namespace VCore
{
    /**
     * Size of one vertex, this must be changed, if there are more or less attributes.
     * Currently the layout is as follows:
     * - Math::Vec3f Position
     * - Math::Vec3f Normal
     * - Math::Vec4f Color
     */
    constexpr static int VertexSize = (sizeof(Math::Vec3f) * 2) + 4; //sizeof(Math::Vec4f);

    //////////////////////////////////////////////////
    // GLTF::CMesh functions
    //////////////////////////////////////////////////

    void GLTF::CMesh::AddPrimitive(uint64_t p_MaterialHandle, const fast_vector<AccessorValue> &p_Accessors)
    {
        auto &primitive = m_Primitives.emplace_back(p_MaterialHandle);
        uint64_t offset = 0;

        for (auto &&value : p_Accessors) 
        {
            // None standard name for the indices accessor.
            if(strcmp(value.first, "INDICES") == 0)
                primitive.IndicesAccessor = m_DocumentRef->AddAccessor(value.second);
            else
            {
                value.second.Offset = offset;

                // Increments the offset, by the type of the current accessor.
                offset += GetTypeSize(value.second);
    
                auto accessorHandle = m_DocumentRef->AddAccessor(value.second);
                primitive.AddAttribute(value.first, accessorHandle);
            }
        }
    }

    //////////////////////////////////////////////////
    // CGLTFExporter functions
    //////////////////////////////////////////////////

    void CGLTFExporter::CloseStream()
    {
        m_IOHandler->Close(m_BinaryStream);
        m_BinaryStream = nullptr;
    }

    void CGLTFExporter::WriteHeaderData(const fast_vector<Mesh> &)
    {
        const auto binaryFilename = GetPathWithoutExt(m_Path) + ".bin";
        m_BinaryStream = m_IOHandler->Open(binaryFilename, "wb");
    }

    void CGLTFExporter::EnterSceneNode(const CSceneNodeBase *p_Node)
    {
        uint64_t rootIdx = static_cast<uint64_t>(-1);
        if(!m_Nodes.empty())
            rootIdx = m_Nodes.top();

        m_Nodes.push(m_Document.CreateNode(p_Node, m_ModelIdDec, rootIdx));
    }

    void CGLTFExporter::LeaveSceneNode(const CSceneNodeBase *)
    {
        m_Nodes.pop();
    }

    void CGLTFExporter::WriteMeshData(const Mesh &p_Mesh)
    {
        auto &mesh = m_Document.CreateMesh();
        for (auto &&surface : p_Mesh->Surfaces) 
        {
            // Firstly create two buffer views for the vertex data and the index data
            const auto vertexBufferSize = surface->GetVertexCount() * VertexSize;
            const auto indexBufferSize = (surface->GetFaceCount() * 3) * sizeof(int);

            const auto vertexBufferView = m_Document.CreateBufferView(vertexBufferSize, m_BinaryStream->Tell(), GLTF::BufferTarget::ARRAY_BUFFER, VertexSize);
            const auto indicesBufferView = m_Document.CreateBufferView(indexBufferSize, m_BinaryStream->Tell() + vertexBufferSize, GLTF::BufferTarget::ELEMENT_ARRAY_BUFFER, 0);

            GLTF::CAccessor positionAccessor(vertexBufferView, GLTF::GLTFTypes::FLOAT, "VEC3", surface->GetVertexCount());

            Math::Vec3f max;
            Math::Vec3f min(INFINITY, INFINITY, INFINITY);

            // Write the data to the blob
            for (size_t i = 0; i < surface->GetVertexCount(); i++)
            {
                auto vertex = surface->GetVertex(i);

                // For the position accessor it is neccessary to know the bounding box.
                max = vertex.Pos.max(max);
                min = vertex.Pos.min(min);

                m_BinaryStream->Write(vertex.Pos);
                m_BinaryStream->Write(vertex.Normal);

                CColor c(vertex.Color);
                uint8_t color[4] = {
                    // Converts the RGB value from sRGB to linear colorspace.
                    static_cast<uint8_t>(pow(static_cast<float>(c.R) / 255.f, 2.2f) * 255.f), 
                    static_cast<uint8_t>(pow(static_cast<float>(c.G) / 255.f, 2.2f) * 255.f), 
                    static_cast<uint8_t>(pow(static_cast<float>(c.B) / 255.f, 2.2f) * 255.f),

                    // Stores the ambient occlussion value inside the alpha channel.
                    // This must be later used in a shader in order to get the ambient occlusion to show.
                    static_cast<uint8_t>((1.0f - (vertex.AmbientOcclusionValue / 3.0)) * 255.f)
                };
                m_BinaryStream->Write(reinterpret_cast<char*>(color), sizeof(color));
            }

            // Writes the index informations.
            m_BinaryStream->Write(static_cast<const char*>(surface->GetRawIndexPointer()), indexBufferSize);

            positionAccessor.SetMin(min);
            positionAccessor.SetMax(max);

            // Create the surface, and sets all needed accessors.
            mesh.AddPrimitive(GetGLTFMaterialHandle(surface->MaterialHandle), {
                { "POSITION",  std::move(positionAccessor) },
                { "NORMAL", GLTF::CAccessor(vertexBufferView, GLTF::GLTFTypes::FLOAT, "VEC3", surface->GetVertexCount()) },
                { "COLOR_0", GLTF::CAccessor(vertexBufferView, GLTF::GLTFTypes::UNSIGNED_BYTE, "VEC4", surface->GetVertexCount()) },
                { "INDICES", GLTF::CAccessor(indicesBufferView, GLTF::GLTFTypes::INT, "SCALAR", surface->GetFaceCount() * 3) }
            });
        }
    }

    void CGLTFExporter::WriteFooterData()
    {
        // Writes padding bytes to the stream, if needed.
        if(Settings->Binary)
        {
            uint64_t padding = 4 - (m_BinaryStream->Tell() % 4);
            for (uint64_t i = 0; i < padding; i++) 
                m_BinaryStream->Write(static_cast<uint8_t>(0));
        }

        const auto binaryFilename = m_BinaryStream->GetFilePath();
        m_Document.AddBuffer(m_BinaryStream->Tell(), Settings->Binary ? "" : GetFilename(binaryFilename));

        // Closes the old binary stream.
        CloseStream();

        CJSON json;
        auto documentJson = json.Serialize(m_Document);
        m_Document.Clear();

        if(!Settings->Binary)
        {
            auto *strm = m_IOHandler->Open(m_Path, "wb");
            strm->Write(documentJson);
            m_IOHandler->Close(strm);
        }
        else
        {
            // Adds padding to the json, so its a multiple of 4.
            uint64_t padding = 4 - (documentJson.size() % 4);
            for (uint64_t i = 0; i < padding; i++)
                documentJson += ' ';

            auto *strm = m_IOHandler->Open(m_Path, "wb");

            // File header
            strm->Write(static_cast<uint32_t>(0x46546C67));                 // GLTF in ASCII
            strm->Write(static_cast<uint32_t>(2));                          // Version
            strm->Write(static_cast<uint32_t>(0));                          // Size of the file, in bytes. Will be patched at the end.

            // Json data
            strm->Write(static_cast<uint32_t>(documentJson.size()));        // Chunk length
            strm->Write(static_cast<uint32_t>(0x4E4F534A));                 // JSON in ASCII
            strm->Write(documentJson);                                      // JSON Data

            m_BinaryStream = m_IOHandler->Open(binaryFilename, "rb");

            // Binary blob
            strm->Write(static_cast<uint32_t>(m_BinaryStream->Size()));     // Chunk length
            strm->Write(static_cast<uint32_t>(0x004E4942));                 // Bin in ASCII

            // Reads the .bin file and writes it's content to the glb file.
            char buffer[4096];
            while (!m_BinaryStream->Eof()) 
            {
                const auto readed = m_BinaryStream->Read(static_cast<char*>(buffer), sizeof(buffer));
                strm->Write(static_cast<char*>(buffer), readed);
            }

            CloseStream();

            // Patches the size of the file, with the real file size.
            auto size = strm->Tell();
            strm->Seek(sizeof(uint32_t) * 2, SeekOrigin::BEG);
            strm->Write(static_cast<uint32_t>(size));

            m_IOHandler->Close(strm);
            m_IOHandler->Delete(binaryFilename);
        }

        m_MaterialHandleMapper.clear();
    }

    uint64_t CGLTFExporter::GetGLTFMaterialHandle(const uint8_t p_MaterialHandle)
    {
        auto it = m_MaterialHandleMapper.find(p_MaterialHandle);
        if(it == m_MaterialHandleMapper.end())
        {
            auto material = MaterialManager::GetMaterial(p_MaterialHandle);
            if(!material)
                material = MaterialManager::GetMaterial(0);

            it = m_MaterialHandleMapper.insert({p_MaterialHandle, m_Document.AddMaterial(material)}).first;
        }

        return it->second;
    }

    // void CGLTFExporter::WriteData(const std::string &p_Path, const std::vector<Mesh> &p_Meshes)
    // {
    //     const auto binaryFilename = GetPathWithoutExt(p_Path) + ".bin";
    //     m_BinaryStream = m_IOHandler->Open(binaryFilename, "wb");

    //     uint64_t animationRootIdx = -1;
    //     for (auto &&mesh : p_Meshes) 
    //     {
    //         if(mesh->Surfaces.empty())
    //             continue;

    //         if(mesh->FrameTime != 0)
    //         {
    //             if(animationRootIdx == static_cast<uint64_t>(-1))
    //                 animationRootIdx = m_Document.CreateNode(GetMeshName(mesh) + "_Anim", -1, Math::Mat4x4(), -1);
    //         }
    //         else
    //             animationRootIdx = -1;

    //         auto meshHandle = WriteMeshData(mesh);
    //         m_Document.CreateNode(GetMeshName(mesh), meshHandle, Settings->WorldSpace ? mesh->ModelMatrix : Math::Mat4x4(), animationRootIdx);
    //     }

    //     // Writes padding bytes to the stream, if needed.
    //     if(Settings->Binary)
    //     {
    //         uint64_t padding = 4 - (m_BinaryStream->Tell() % 4);
    //         for (uint64_t i = 0; i < padding; i++) 
    //             m_BinaryStream->Write(static_cast<uint8_t>(0));
    //     }

    //     m_Document.AddBuffer(m_BinaryStream->Tell(), Settings->Binary ? "" : GetFilename(binaryFilename));

    //     // Closes the old binary stream.
    //     CloseStream();

    //     CJSON json;
    //     auto documentJson = json.Serialize(m_Document);
    //     m_Document.Clear();

    //     if(!Settings->Binary)
    //     {
    //         auto *strm = m_IOHandler->Open(p_Path, "wb");
    //         strm->Write(documentJson);
    //         m_IOHandler->Close(strm);
    //     }
    //     else
    //     {
    //         // Adds padding to the json, so its a multiple of 4.
    //         uint64_t padding = 4 - (documentJson.size() % 4);
    //         for (uint64_t i = 0; i < padding; i++)
    //             documentJson += ' ';

    //         auto *strm = m_IOHandler->Open(p_Path, "wb");

    //         // File header
    //         strm->Write(static_cast<uint32_t>(0x46546C67));                 // GLTF in ASCII
    //         strm->Write(static_cast<uint32_t>(2));                          // Version
    //         strm->Write(static_cast<uint32_t>(0));                          // Size of the file, in bytes. Will be patched at the end.

    //         // Json data
    //         strm->Write(static_cast<uint32_t>(documentJson.size()));        // Chunk length
    //         strm->Write(static_cast<uint32_t>(0x4E4F534A));                 // JSON in ASCII
    //         strm->Write(documentJson);                                      // JSON Data

    //         m_BinaryStream = m_IOHandler->Open(binaryFilename, "rb");

    //         // Binary blob
    //         strm->Write(static_cast<uint32_t>(m_BinaryStream->Size()));     // Chunk length
    //         strm->Write(static_cast<uint32_t>(0x004E4942));                 // Bin in ASCII

    //         // Reads the .bin file and writes it's content to the glb file.
    //         char buffer[4096];
    //         while (!m_BinaryStream->Eof()) 
    //         {
    //             const auto readed = m_BinaryStream->Read(static_cast<char*>(buffer), sizeof(buffer));
    //             strm->Write(static_cast<char*>(buffer), readed);
    //         }

    //         CloseStream();

    //         // Patches the size of the file, with the real file size.
    //         auto size = strm->Tell();
    //         strm->Seek(sizeof(uint32_t) * 2, SeekOrigin::BEG);
    //         strm->Write(static_cast<uint32_t>(size));

    //         m_IOHandler->Close(strm);
    //         m_IOHandler->Delete(binaryFilename);
    //     }

    //     m_MaterialHandleMapper.clear();
    // }
}  // namespace VCore
