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

namespace VCore
{
    std::map<std::string, std::vector<char>> CGLTFExporter::Generate(std::vector<Mesh> Meshes)
    {
        std::vector<GLTF::CBufferView> bufferViews;
        std::vector<GLTF::CAccessor> accessors;
        std::vector<GLTF::CMaterial> materials;

        std::vector<char> binary;

        std::vector<GLTF::CNode> nodes;
        std::vector<GLTF::CMesh> glTFMeshes;
        size_t matId = 0;

        for (auto &&mesh : Meshes)
        {
            GLTF::CMesh GLTFMesh;
            nodes.push_back(GLTF::CNode(glTFMeshes.size(), m_Settings->WorldSpace ? mesh->ModelMatrix : Math::Mat4x4()));

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

                surfaceVerticesView.Size = surface.Vertices.size() * sizeof(SVertex);
                surfaceVerticesView.Target = GLTF::BufferTarget::ARRAY_BUFFER;
                surfaceVerticesView.ByteStride = sizeof(SVertex);
                surfaceVerticesView.Offset = binary.size();

                indexView.Offset = surfaceVerticesView.Offset + surfaceVerticesView.Size;//uv.Size;
                indexView.Size = surface.Indices.size() * sizeof(int);
                indexView.Target = GLTF::BufferTarget::ELEMENT_ARRAY_BUFFER;

                for (auto &&v : surface.Vertices)
                {
                    max = v.Pos.max(max);
                    min = v.Pos.min(min);
                }

                GLTF::CAccessor positionAccessor, normalAccessor, uvAccessor, indexAccessor;
                positionAccessor.BufferView = bufferViews.size();
                positionAccessor.ComponentType = GLTF::GLTFTypes::FLOAT;
                positionAccessor.Count = surface.Vertices.size();
                positionAccessor.Type = "VEC3";
                positionAccessor.SetMin(min);
                positionAccessor.SetMax(max);

                normalAccessor.BufferView = bufferViews.size();
                normalAccessor.ComponentType = GLTF::GLTFTypes::FLOAT;
                normalAccessor.Count = surface.Vertices.size();
                normalAccessor.Type = "VEC3";
                normalAccessor.Offset = sizeof(Math::Vec3f);

                uvAccessor.BufferView = bufferViews.size();
                uvAccessor.ComponentType = GLTF::GLTFTypes::FLOAT;
                uvAccessor.Count = surface.Vertices.size();
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

                memcpy(binary.data() + pos, surface.Vertices.data(), surfaceVerticesView.Size);
                pos += surfaceVerticesView.Size;

                memcpy(binary.data() + pos, surface.Indices.data(), indexView.Size);
            }
        
            glTFMeshes.push_back(GLTFMesh);
        }
        
        std::vector<GLTF::CImage> Images;
        GLTF::CBuffer Buffer;

        auto textures = Meshes[0]->Textures;

        // For glb add padding to satify the 4 Byte boundary.
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
            Image.Uri = m_ExternalFilenames + ".albedo.png";
            Images.push_back(Image);

            if(textures.find(TextureType::EMISSION) != textures.end())
            {
                GLTF::CImage Image;
                Image.Uri = m_ExternalFilenames + ".emission.png";
                Images.push_back(Image);
            }

            Buffer.Uri = m_ExternalFilenames + ".bin";
        }
            

        Buffer.Size = binary.size(); 

        CJSON json;
        json.AddPair("asset", GLTF::CAsset());
        json.AddPair("scene", 0);
        json.AddPair("scenes", std::vector<GLTF::CScene>() = { GLTF::CScene(nodes.size()) }); // Erweitern um nodes
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

        // Padding for the binary format
        if(m_Settings->Binary)
        {
            int Padding = 4 - (JS.size() % 4);
            for (int i = 0; i < Padding; i++)
                JS += ' ';
        }

        if(!m_Settings->Binary)
        {
            std::vector<char> GLTF(JS.begin(), JS.end());
            std::map<std::string, std::vector<char>> ret = 
            {
                {"gltf", GLTF},
                {"bin", binary},
                {"albedo.png", textures[TextureType::DIFFIUSE]->AsPNG()}
            };

            if(textures.find(TextureType::EMISSION) != textures.end())
                ret["emission.png"] = textures[TextureType::EMISSION]->AsPNG();

            return ret;
        }

        // Builds the binary buffer.
        std::vector<char> GLB(sizeof(uint32_t) * 3 + sizeof(uint32_t) * 2 + JS.size() + sizeof(uint32_t) * 2 + binary.size(), '\0');
        uint32_t Magic = 0x46546C67; //GLTF in ASCII
        uint32_t Version = 2;
        uint32_t Length = GLB.size();

        memcpy(GLB.data(), &Magic, sizeof(uint32_t));
        memcpy(GLB.data() + sizeof(uint32_t), &Version, sizeof(uint32_t));
        memcpy(GLB.data() + sizeof(uint32_t) * 2, &Length, sizeof(uint32_t));

        size_t Pos = sizeof(uint32_t) * 3; 

        uint32_t ChunkLength = JS.size();
        uint32_t ChunkType = 0x4E4F534A; //JSON in ASCII

        memcpy(GLB.data() + Pos, &ChunkLength, sizeof(uint32_t));
        memcpy(GLB.data() + Pos + sizeof(uint32_t), &ChunkType, sizeof(uint32_t));
        memcpy(GLB.data() + Pos + sizeof(uint32_t) * 2, JS.data(), JS.size());

        Pos += sizeof(uint32_t) * 2 + JS.size(); 

        ChunkLength = binary.size();
        ChunkType = 0x004E4942; //Bin in ASCII

        memcpy(GLB.data() + Pos, &ChunkLength, sizeof(uint32_t));
        memcpy(GLB.data() + Pos + sizeof(uint32_t), &ChunkType, sizeof(uint32_t));
        memcpy(GLB.data() + Pos + sizeof(uint32_t) * 2, binary.data(), binary.size());

        return {
            {"glb", GLB}
        };
    }
}
