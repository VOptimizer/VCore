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
#include <string.h>
#include <sstream>
#include "WavefrontObjExporter.hpp"
#include "../../FileUtils.hpp"

namespace VCore
{   
    // std::map<std::string, std::vector<char>> CWavefrontObjExporter::Generate(std::vector<Mesh> Meshes)
    void CWavefrontObjExporter::WriteData(const std::string &_Path, const std::vector<Mesh> &_Meshes)
    {
        std::string filenameWithoutExt = GetFilenameWithoutExt(_Path);
        std::string filePathWithoutExt = GetPathWithoutExt(_Path);

        auto objFile = m_IOHandler->Open(filePathWithoutExt + ".obj", "wb");
        auto mtlFile = m_IOHandler->Open(filePathWithoutExt + ".mtl", "wb");

        objFile->Write("# Generated with VCore (https://github.com/VOptimizer/VCore)\n");
        objFile->Write("# These comments can be removed\n");
        objFile->Write("mtllib " + filenameWithoutExt + ".mtl\n");

        size_t meshCounter = 0;
        size_t matCounter = 0;
        int indexOffset = 0;

        for (auto &&mesh : _Meshes)
        {
            objFile->Write("o " + GetMeshName(mesh, "VoxelModel" + std::to_string(meshCounter)) + "\n");
            meshCounter++;

            for (auto &&surface : mesh->Surfaces)
            {
                //"Extracts" the rotation matrix. Quick'n Dirty
                Math::Mat4x4 rotMat = mesh->ModelMatrix;
                rotMat.x.w = 0;
                rotMat.y.w = 0;
                rotMat.z.w = 0;

                // Lazy memory management.
                {
                    std::stringstream vertexList, uvList, normalList;
                    for(int i = 0; i < surface->GetVertexCount(); i++)
                    {
                        auto v = surface->GetVertex(i);
                        Math::Vec3f pos = v.Pos;
                        Math::Vec3f normal = v.Normal;

                        if(Settings->WorldSpace)
                        {
                            pos = mesh->ModelMatrix * pos;
                            normal = rotMat * normal;
                        }

                        vertexList << "v " << pos.x << " " << pos.y << " " << pos.z << std::endl;
                        normalList << "vn " << normal.x << " " << normal.y << " " << normal.z << std::endl;
                        uvList << "vt " << v.UV.x << " " << v.UV.y << std::endl;
                    }

                    objFile->Write(vertexList.str() + "\n");
                    objFile->Write(normalList.str() + "\n");
                    objFile->Write(uvList.str() + "\n");
                }
                
                size_t matID = 0;
                matID = matCounter;

                float ambient = 1.0;
                int illum = 2;
                float transparency = 0;
                float alpha = 1.0;

                if(surface->FaceMaterial->Metallic != 0.0)
                {
                    ambient = surface->FaceMaterial->Metallic;
                    illum = 3;
                }
                else if(surface->FaceMaterial->Transparency != 0.0) // Glass
                {
                    illum = 4;
                    transparency = surface->FaceMaterial->Transparency;
                    alpha = 1 - surface->FaceMaterial->Transparency;
                }

                mtlFile->Write("newmtl Mat" + std::to_string(matCounter) + "\n");
                mtlFile->Write("Ns " + std::to_string(surface->FaceMaterial->Roughness * 1000.f) + "\n");
                mtlFile->Write("Ka " + std::to_string(ambient) + " " + std::to_string(ambient) + " " + std::to_string(ambient) + "\n");
                mtlFile->Write("Kd 1.0 1.0 1.0\n");
                mtlFile->Write("Ks " + std::to_string(surface->FaceMaterial->Specular) + " " + std::to_string(surface->FaceMaterial->Specular) + " "  + std::to_string(surface->FaceMaterial->Specular) + "\n");
                
                if(surface->FaceMaterial->Power != 0.0)
                {
                    mtlFile->Write("Ke " + std::to_string(surface->FaceMaterial->Power) + " " + std::to_string(surface->FaceMaterial->Power) + " " + std::to_string(surface->FaceMaterial->Power) + "\n");
                    mtlFile->Write("map_Ke " + filenameWithoutExt + ".emission.png\n");
                }

                mtlFile->Write("Tr " + std::to_string(transparency) + "\n");
                mtlFile->Write("d " + std::to_string(alpha) + "\n");
                mtlFile->Write("Ni " + std::to_string(surface->FaceMaterial->IOR) + "\n");
                mtlFile->Write("illum " + std::to_string(illum) + "\n");
                mtlFile->Write("map_Kd " + filenameWithoutExt + ".albedo.png\n");

                matCounter++;  
                
                objFile->Write("usemtl Mat" + std::to_string(matID) + "\n");

                for (uint64_t i = 0; i < surface->GetFaceCount(); i++)
                {
                    objFile->Write("f");
                    for (char j = 0; j < 3; j++)
                    {
                        int index = surface->GetIndex(i * 3 + j) + indexOffset + 1;

                        objFile->Write(" ");
                        objFile->Write(std::to_string(index) + "/" + std::to_string(index) + "/" + std::to_string(index));
                    }
                    objFile->Write("\n");
                }
                indexOffset += surface->GetVertexCount();
            }
        }

        m_IOHandler->Close(objFile);
        m_IOHandler->Close(mtlFile);

        // Write all textures to disk.
        auto textures = _Meshes[0]->Textures;
        SaveTexture(textures[TextureType::DIFFIUSE], _Path, "albedo");
        if(textures.find(TextureType::EMISSION) != textures.end())
            SaveTexture(textures[TextureType::EMISSION], _Path, "emission");
    }
}
