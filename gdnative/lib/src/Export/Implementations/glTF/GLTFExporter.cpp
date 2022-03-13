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
#include "../GLTFExporter.hpp"

namespace VoxelOptimizer
{
    std::map<std::string, std::vector<char>> CGLTFExporter::Generate(std::vector<Mesh> Meshes)
    {
        std::vector<GLTF::CBufferView> BufferViews;
        std::vector<GLTF::CAccessor> Accessors;
        std::vector<GLTF::CMaterial> Materials;

        std::vector<char> Binary;

        std::vector<GLTF::CNode> Nodes;
        std::vector<GLTF::CMesh> GLTFMeshes;
        std::map<int, bool> processedMaterials;

        for (auto &&mesh : Meshes)
        {
            GLTF::CMesh GLTFMesh;
            Nodes.push_back(GLTF::CNode(GLTFMeshes.size(), m_Settings->WorldSpace ? mesh->ModelMatrix : CMat4x4()));

            for (auto &&f : mesh->Faces)
            {
                std::vector<CVector> Vertices, Normals, UVs;
                std::vector<int> Indices;
                std::map<CVector, int> IndicesIndex;

                CVector Max, Min(10000, 10000, 10000);

                // Add only "new" materials.
                if(processedMaterials.find(f->MaterialIndex) == processedMaterials.end())
                {
                    processedMaterials.insert({f->MaterialIndex, true});

                    GLTF::CMaterial Mat;
                    Mat.Name = "Mat" + std::to_string(f->MaterialIndex);
                    Mat.Metallic = f->FaceMaterial->Metallic;
                    Mat.Roughness = f->FaceMaterial->Roughness;
                    Mat.Emissive = f->FaceMaterial->Power;
                    Mat.Transparency = f->FaceMaterial->Transparency;
                    Materials.push_back(Mat);
                }

                for (size_t i = 0; i < f->Indices.size(); i += 3)
                {
                    for (size_t j = 0; j < 3; j++)
                    {
                        CVector vec = f->Indices[i + j];

                        int Index = 0;

                        auto IT = IndicesIndex.find(vec);
                        if(IT == IndicesIndex.end())
                        {
                            Vertices.push_back(mesh->Vertices[(size_t)vec.x - 1]);
                            Normals.push_back(mesh->Normals[(size_t)vec.y - 1]);
                            UVs.push_back(mesh->UVs[(size_t)vec.z - 1]);

                            Min = Min.Min(mesh->Vertices[(size_t)vec.x - 1]);
                            Max = Max.Max(mesh->Vertices[(size_t)vec.x - 1]);

                            Index = Vertices.size() - 1;
                            IndicesIndex.insert({vec, Index});
                        }
                        else
                            Index = IT->second;

                        Indices.push_back(Index);
                    }
                }

                GLTF::CBufferView Position, Normal, UV, IndexView;

                Position.Offset = Binary.size();
                Position.Size = Vertices.size() * sizeof(CVector);

                Normal.Offset = Position.Offset + Position.Size;
                Normal.Size = Normals.size() * sizeof(CVector);

                UV.Offset = Normal.Offset + Normal.Size;
                UV.Size = UVs.size() * (sizeof(float) * 2);

                IndexView.Offset = UV.Offset + UV.Size;
                IndexView.Size = Indices.size() * sizeof(int);

                GLTF::CAccessor PositionAccessor, NormalAccessor, UVAccessor, IndexAccessor;
                PositionAccessor.BufferView = BufferViews.size();
                PositionAccessor.ComponentType = GLTF::GLTFTypes::FLOAT;
                PositionAccessor.Count = Vertices.size();
                PositionAccessor.Type = "VEC3";
                PositionAccessor.SetMin(Min);
                PositionAccessor.SetMax(Max);

                NormalAccessor.BufferView = BufferViews.size() + 1;
                NormalAccessor.ComponentType = GLTF::GLTFTypes::FLOAT;
                NormalAccessor.Count = Normals.size();
                NormalAccessor.Type = "VEC3";

                UVAccessor.BufferView = BufferViews.size() + 2;
                UVAccessor.ComponentType = GLTF::GLTFTypes::FLOAT;
                UVAccessor.Count = UVs.size();
                UVAccessor.Type = "VEC2";

                IndexAccessor.BufferView = BufferViews.size() + 3;
                IndexAccessor.ComponentType = GLTF::GLTFTypes::INT;
                IndexAccessor.Count = Indices.size();
                IndexAccessor.Type = "SCALAR";

                GLTF::CPrimitive Primitive;
                Primitive.PositionAccessor = Accessors.size();
                Primitive.NormalAccessor = Accessors.size() + 1;
                Primitive.TextCoordAccessor = Accessors.size() + 2;
                Primitive.IndicesAccessor = Accessors.size() + 3;
                Primitive.Material = f->MaterialIndex; //GLTFMesh.Primitives.size();

                GLTFMesh.Primitives.push_back(Primitive);

                BufferViews.push_back(Position);
                BufferViews.push_back(Normal);
                BufferViews.push_back(UV);
                BufferViews.push_back(IndexView);

                Accessors.push_back(PositionAccessor);
                Accessors.push_back(NormalAccessor);
                Accessors.push_back(UVAccessor);
                Accessors.push_back(IndexAccessor);

                size_t Pos = Binary.size();

                Binary.resize(Binary.size() + Position.Size + Normal.Size + UV.Size + IndexView.Size);

                memcpy(Binary.data() + Pos, Vertices.data(), Position.Size);
                Pos += Position.Size;

                memcpy(Binary.data() + Pos, Normals.data(), Normal.Size);
                Pos += Normal.Size;

                for (auto &&uv : UVs)
                {
                    memcpy(Binary.data() + Pos, &uv, sizeof(float) * 2);
                    Pos += sizeof(float) * 2;
                }

                memcpy(Binary.data() + Pos, Indices.data(), IndexView.Size);
            }
        
            GLTFMeshes.push_back(GLTFMesh);
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

            size_t Size = Binary.size();
            int Padding = (Binary.size() + diffuse.size() + emission.size()) % 4;

            Binary.resize(Binary.size() + diffuse.size() + emission.size() + Padding, '\0');
            memcpy(Binary.data() + Size, diffuse.data(), diffuse.size());
            memcpy(Binary.data() + Size + diffuse.size(), emission.data(), emission.size());

            GLTF::CBufferView ImageView;
            ImageView.Offset = Size;
            ImageView.Size = diffuse.size();

            GLTF::CImage Image;
            Image.BufferView = BufferViews.size();
            BufferViews.push_back(ImageView);
            Images.push_back(Image);

            if(!emission.empty())
            {
                GLTF::CBufferView ImageView;
                ImageView.Offset = Size + diffuse.size();
                ImageView.Size = emission.size();

                GLTF::CImage Image;
                Image.BufferView = BufferViews.size();
                BufferViews.push_back(ImageView);
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
            

        Buffer.Size = Binary.size(); 

        CJSON json;
        json.AddPair("asset", GLTF::CAsset());
        json.AddPair("scene", 0);
        json.AddPair("scenes", std::vector<GLTF::CScene>() = { GLTF::CScene(Nodes.size()) }); // Erweitern um nodes
        json.AddPair("nodes", Nodes);

        json.AddPair("meshes", GLTFMeshes);
        json.AddPair("accessors", Accessors);
        json.AddPair("bufferViews", BufferViews);
        json.AddPair("materials", Materials);        

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
            int Padding = JS.size() % 4;
            for (int i = 0; i < Padding; i++)
                JS += ' ';
        }

        if(!m_Settings->Binary)
        {
            std::vector<char> GLTF(JS.begin(), JS.end());
            std::map<std::string, std::vector<char>> ret = 
            {
                {"gltf", GLTF},
                {"bin", Binary},
                {"albedo.png", textures[TextureType::DIFFIUSE]->AsPNG()}
            };

            if(textures.find(TextureType::EMISSION) != textures.end())
                ret["emission.png"] = textures[TextureType::EMISSION]->AsPNG();

            return ret;
        }

        // Builds the binary buffer.
        std::vector<char> GLB(sizeof(uint32_t) * 3 + sizeof(uint32_t) * 2 + JS.size() + sizeof(uint32_t) * 2 + Binary.size(), '\0');
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

        ChunkLength = Binary.size();
        ChunkType = 0x004E4942; //Bin in ASCII

        memcpy(GLB.data() + Pos, &ChunkLength, sizeof(uint32_t));
        memcpy(GLB.data() + Pos + sizeof(uint32_t), &ChunkType, sizeof(uint32_t));
        memcpy(GLB.data() + Pos + sizeof(uint32_t) * 2, Binary.data(), Binary.size());

        return {
            {"glb", GLB}
        };
    }
} // namespace VoxelOptimizer