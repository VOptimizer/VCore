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
#include <sstream>
#include <string.h>
#include "WavefrontObjExporter.hpp"

namespace VCore
{   
    std::map<std::string, std::vector<char>> CWavefrontObjExporter::Generate(std::vector<Mesh> Meshes)
    {
        std::stringstream objFile, mtlFile;
        if(m_ExternalFilenames.empty())
            m_ExternalFilenames = "materials";

        objFile << "# Generated with VCore" << std::endl;
        objFile << "# These comments can be removed" << std::endl;
        objFile << "mtllib " << m_ExternalFilenames << ".mtl" << std::endl;

        size_t meshCounter = 0;
        size_t matCounter = 0;
        int indicesOffset = 0;

        for (auto &&mesh : Meshes)
        {
            objFile << "o VoxelMesh" << meshCounter << std::endl;
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
                    for(int i = 0; i < surface.Size(); i++)
                    {
                        auto v = surface[i];
                        Math::Vec3f pos = v.Pos;
                        Math::Vec3f normal = v.Normal;

                        if(m_Settings->WorldSpace)
                        {
                            pos = mesh->ModelMatrix * pos;
                            normal = rotMat * normal;
                        }

                        vertexList << "v " << pos.x << " " << pos.y << " " << pos.z << std::endl;
                        normalList << "vn " << normal.x << " " << normal.y << " " << normal.z << std::endl;
                        uvList << "vt " << v.UV.x << " " << v.UV.y << std::endl;
                    }

                    objFile << vertexList.rdbuf() << normalList.rdbuf() << uvList.rdbuf();
                }
                
                size_t matID = 0;
                matID = matCounter;

                float ambient = 1.0;
                int illum = 2;
                float transparency = 0;
                float alpha = 1.0;

                if(surface.FaceMaterial->Metallic != 0.0)
                {
                    ambient = surface.FaceMaterial->Metallic;
                    illum = 3;
                }
                else if(surface.FaceMaterial->Transparency != 0.0) // Glass
                {
                    illum = 4;
                    transparency = surface.FaceMaterial->Transparency;
                    alpha = 1 - surface.FaceMaterial->Transparency;
                }

                mtlFile << "newmtl Mat" << matCounter << std::endl;
                mtlFile << "Ns " << surface.FaceMaterial->Roughness * 1000.f << std::endl;
                mtlFile << "Ka " << ambient << " " << ambient << " " << ambient << std::endl;
                mtlFile << "Kd 1.0 1.0 1.0" << std::endl;
                mtlFile << "Ks " << surface.FaceMaterial->Specular << " " << surface.FaceMaterial->Specular << " " << surface.FaceMaterial->Specular << std::endl;
                
                if(surface.FaceMaterial->Power != 0.0)
                {
                    mtlFile << "Ke " << surface.FaceMaterial->Power << " " << surface.FaceMaterial->Power << " " << surface.FaceMaterial->Power << std::endl;
                    mtlFile << "map_Ke " << m_ExternalFilenames << ".emission.png" << std::endl;
                }

                mtlFile << "Tr " << transparency << std::endl;
                mtlFile << "d " << alpha << std::endl;
                mtlFile << "Ni " << surface.FaceMaterial->IOR << std::endl;
                mtlFile << "illum " << illum << std::endl;
                mtlFile << "map_Kd " << m_ExternalFilenames << ".albedo.png" << std::endl;

                matCounter++;  
                
                objFile << "usemtl Mat" << matID << std::endl;

                for (size_t i = 0; i < surface.Indices.size(); i += 3)
                {
                    objFile << "f";
                    for (char j = 0; j < 3; j++)
                    {
                        int index = surface.Indices[i + j] + indicesOffset + 1;

                        objFile << " ";
                        objFile << index << "/" << index << "/" << index;
                    }
                    objFile << std::endl;
                }
                indicesOffset += surface.Indices.size();
            }
        }

        std::string objFileStr = objFile.str();
        std::string mtlFileStr = mtlFile.str();

        auto textures = Meshes[0]->Textures;

        std::map<std::string, std::vector<char>> ret = 
        {
            {"obj", std::vector<char>(objFileStr.begin(), objFileStr.end())},
            {"mtl", std::vector<char>(mtlFileStr.begin(), mtlFileStr.end())},
            {"albedo.png", textures[TextureType::DIFFIUSE]->AsPNG()}
        };

        if(textures.find(TextureType::EMISSION) != textures.end())
            ret["emission.png"] = textures[TextureType::EMISSION]->AsPNG();

        return ret;
    }
}
