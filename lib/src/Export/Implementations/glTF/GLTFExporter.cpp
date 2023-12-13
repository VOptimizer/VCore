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

#include <algorithm>
#include <fstream>
#include "Nodes.hpp"
#include <sstream>
#include <string.h>
#include "GLTFExporter.hpp"
#include "../../../FileUtils.hpp"

namespace VCore
{
    void CGLTFExporter::WriteData(const std::string &_Path, const std::vector<Mesh> &_Meshes)
    {
        auto filenameWithoutExt = GetFilenameWithoutExt(_Path);

        std::vector<GLTF::CBufferView> bufferViews;
        std::vector<GLTF::CAccessor> accessors;
        std::vector<GLTF::CMaterial> materials;

        std::vector<char> binary;

        std::vector<GLTF::CNode> nodes;
        std::vector<GLTF::CMesh> glTFMeshes;
        std::vector<int> rootNodes;
        size_t matId = 0;

        size_t animationRootIdx = -1;

        for (auto &&mesh : _Meshes)
        {
            GLTF::CMesh GLTFMesh;
            if(mesh->FrameTime != 0)
            {
                if(animationRootIdx == ((size_t)-1))
                {
                    animationRootIdx = nodes.size();
                    nodes.push_back(GLTF::CNode(mesh->Name + "_Anim"));
                    rootNodes.push_back(animationRootIdx);
                }

                nodes[animationRootIdx].AddChild(nodes.size());
            }
            else
            {
                animationRootIdx = -1;
                rootNodes.push_back(nodes.size());
            }

            nodes.push_back(GLTF::CNode(GetMeshName(mesh), glTFMeshes.size(), m_Settings->WorldSpace ? mesh->ModelMatrix : Math::Mat4x4()));

            for (auto &&surface : mesh->Surfaces)
            {
                Math::Vec3f max, min(10000, 10000, 10000);

                GLTF::CMaterial Mat;
                Mat.Name = "Mat" + std::to_string(matId + 1);
                Mat.Metallic = surface.FaceMaterial->Metallic;
                Mat.Roughness = surface.FaceMaterial->Roughness;
                Mat.Emissive = surface.FaceMaterial->Power;
                Mat.Transparency = surface.FaceMaterial->Transparency;
                materials.push_back(Mat);

                GLTF::CBufferView surfaceVerticesView, indexView;

                auto vertices = surface.GetVertices();

                surfaceVerticesView.Size = vertices.size() * sizeof(SVertex);
                surfaceVerticesView.Target = GLTF::BufferTarget::ARRAY_BUFFER;
                surfaceVerticesView.ByteStride = sizeof(SVertex);
                surfaceVerticesView.Offset = binary.size();

                indexView.Offset = surfaceVerticesView.Offset + surfaceVerticesView.Size;//uv.Size;
                indexView.Size = surface.Indices.size() * sizeof(int);
                indexView.Target = GLTF::BufferTarget::ELEMENT_ARRAY_BUFFER;

                for (auto &&v : vertices)
                {
                    max = v.Pos.max(max);
                    min = v.Pos.min(min);
                }

                GLTF::CAccessor positionAccessor, normalAccessor, uvAccessor, indexAccessor;
                positionAccessor.BufferView = bufferViews.size();
                positionAccessor.ComponentType = GLTF::GLTFTypes::FLOAT;
                positionAccessor.Count = vertices.size();
                positionAccessor.Type = "VEC3";
                positionAccessor.SetMin(min);
                positionAccessor.SetMax(max);

                normalAccessor.BufferView = bufferViews.size();
                normalAccessor.ComponentType = GLTF::GLTFTypes::FLOAT;
                normalAccessor.Count = vertices.size();
                normalAccessor.Type = "VEC3";
                normalAccessor.Offset = sizeof(Math::Vec3f);

                uvAccessor.BufferView = bufferViews.size();
                uvAccessor.ComponentType = GLTF::GLTFTypes::FLOAT;
                uvAccessor.Count = vertices.size();
                uvAccessor.Type = "VEC2";
                uvAccessor.Offset = normalAccessor.Offset + sizeof(Math::Vec3f);

                indexAccessor.BufferView = bufferViews.size() + 1;
                indexAccessor.ComponentType = GLTF::GLTFTypes::INT;
                indexAccessor.Count = surface.Indices.size();
                indexAccessor.Type = "SCALAR";

                GLTF::CPrimitive Primitive;
                Primitive.PositionAccessor = accessors.size();
                Primitive.NormalAccessor = accessors.size() + 1;
                Primitive.TextCoordAccessor = accessors.size() + 2;
                Primitive.IndicesAccessor = accessors.size() + 3;
                Primitive.Material = matId;
                matId++;

                GLTFMesh.Primitives.push_back(Primitive);

                bufferViews.push_back(surfaceVerticesView);
                bufferViews.push_back(indexView);

                accessors.push_back(positionAccessor);
                accessors.push_back(normalAccessor);
                accessors.push_back(uvAccessor);
                accessors.push_back(indexAccessor);

                size_t pos = binary.size();

                binary.resize(binary.size() + surfaceVerticesView.Size + indexView.Size);

                memcpy(binary.data() + pos, vertices.data(), surfaceVerticesView.Size);
                pos += surfaceVerticesView.Size;

                memcpy(binary.data() + pos, surface.Indices.data(), indexView.Size);
            }
        
            glTFMeshes.push_back(GLTFMesh);
        }
        
        std::vector<GLTF::CImage> Images;
        GLTF::CBuffer Buffer;

        auto textures = _Meshes[0]->Textures;

        // For glb add padding to satisfy the 4 Byte boundary.
        if(m_Settings->Binary)
        {          
            std::vector<char> diffuse, emission;
            diffuse = textures[TextureType::DIFFIUSE]->AsPNG();

            if(textures.find(TextureType::EMISSION) != textures.end())
                emission = textures[TextureType::EMISSION]->AsPNG();

            size_t Size = binary.size();
            int Padding = 4 - ((binary.size() + diffuse.size() + emission.size()) % 4);

            binary.resize(binary.size() + diffuse.size() + emission.size() + Padding, '\0');
            memcpy(binary.data() + Size, diffuse.data(), diffuse.size());
            memcpy(binary.data() + Size + diffuse.size(), emission.data(), emission.size());

            GLTF::CBufferView ImageView;
            ImageView.Offset = Size;
            ImageView.Size = diffuse.size();

            GLTF::CImage Image;
            Image.BufferView = bufferViews.size();
            bufferViews.push_back(ImageView);
            Images.push_back(Image);

            if(!emission.empty())
            {
                GLTF::CBufferView ImageView;
                ImageView.Offset = Size + diffuse.size();
                ImageView.Size = emission.size();

                GLTF::CImage Image;
                Image.BufferView = bufferViews.size();
                bufferViews.push_back(ImageView);
                Images.push_back(Image);
            }
        }
        else
        {
            GLTF::CImage Image;
            Image.Uri = filenameWithoutExt + ".albedo.png";
            Images.push_back(Image);

            if(textures.find(TextureType::EMISSION) != textures.end())
            {
                GLTF::CImage Image;
                Image.Uri = filenameWithoutExt + ".emission.png";
                Images.push_back(Image);
            }

            Buffer.Uri = filenameWithoutExt + ".bin";
        }
            

        Buffer.Size = binary.size(); 

        CJSON json;
        json.AddPair("asset", GLTF::CAsset());
        json.AddPair("scene", 0);
        json.AddPair("scenes", std::vector<GLTF::CScene>() = { GLTF::CScene(std::move(rootNodes)) });
        json.AddPair("nodes", nodes);

        json.AddPair("meshes", glTFMeshes);
        json.AddPair("accessors", accessors);
        json.AddPair("bufferViews", bufferViews);
        json.AddPair("materials", materials);        

        json.AddPair("images", Images);

        std::vector<GLTF::CTexture> gltfTextures = { GLTF::CTexture() };
        if(textures.find(TextureType::EMISSION) != textures.end())
            gltfTextures.push_back(GLTF::CTexture(1));

        json.AddPair("textures", gltfTextures);   
        json.AddPair("buffers", std::vector<GLTF::CBuffer>() = { Buffer });
        
        std::string JS = json.Serialize();
        if(!m_Settings->Binary)
        {
            auto strm = m_IOHandler->Open(_Path, "wb");
            strm->Write(JS);
            m_IOHandler->Close(strm);

            strm = m_IOHandler->Open(GetPathWithoutExt(_Path) + ".bin", "wb");
            strm->Write(binary.data(), binary.size());
            m_IOHandler->Close(strm);

            SaveTexture(textures[TextureType::DIFFIUSE], _Path, "albedo");
            if(textures.find(TextureType::EMISSION) != textures.end())
                SaveTexture(textures[TextureType::EMISSION], _Path, "emission");
        }
        else
        {
            // Adds padding to the json, so its a multiple of 4.
            int Padding = 4 - (JS.size() % 4);
            for (int i = 0; i < Padding; i++)
                JS += ' ';

            auto strm = m_IOHandler->Open(_Path, "wb");

            // File header
            strm->Write((uint32_t)0x46546C67);  // GLTF in ASCII
            strm->Write((uint32_t)2);  // Version
            strm->Write((uint32_t)(sizeof(uint32_t) * 3 + sizeof(uint32_t) * 2 + JS.size() + sizeof(uint32_t) * 2 + binary.size())); // Total file size.

            // Json data
            strm->Write((uint32_t)JS.size());   // Chunk length
            strm->Write((uint32_t)0x4E4F534A);  // JSON in ASCII
            strm->Write(JS);                    // JSON Data

            // Binary blob
            strm->Write((uint32_t)binary.size());   // Chunk length
            strm->Write((uint32_t)0x004E4942);  // Bin in ASCII
            strm->Write(binary.data(), binary.size()); // Bin Data

            m_IOHandler->Close(strm);
        }
    }
}
