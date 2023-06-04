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

namespace VoxelOptimizer
{   
    std::map<std::string, std::vector<char>> CWavefrontObjExporter::Generate(std::vector<Mesh> Meshes)
    {
        std::stringstream ObjFile, MTLFile;
        if(m_ExternalFilenames.empty())
            m_ExternalFilenames = "materials";

        ObjFile << "# Generated with VoxelOptimizer" << std::endl;
        ObjFile << "# These comments can be removed" << std::endl;
        ObjFile << "mtllib " << m_ExternalFilenames << ".mtl" << std::endl;

        size_t meshCounter = 0;
        size_t MatCounter = 0;
        std::map<int, size_t> processedMaterials;
        Math::Vec3f indicesOffset;

        for (auto &&mesh : Meshes)
        {
            ObjFile << "o VoxelMesh" << meshCounter << std::endl;
            meshCounter++;

            for (auto &&v : mesh->Vertices)
            {
                Math::Vec3f tmp = v;
                if(m_Settings->WorldSpace)
                    tmp = mesh->ModelMatrix * v;

                ObjFile << "v " << tmp.x << " " << tmp.y << " " << tmp.z << std::endl;
            }

            //"Extracts" the rotation matrix. Quick'n Dirty
            Math::Mat4x4 rotMat = mesh->ModelMatrix;
            rotMat.x.w = 0;
            rotMat.y.w = 0;
            rotMat.z.w = 0;

            for (auto &&vn : mesh->Normals)
            {
                Math::Vec3f tmp = vn;
                if(m_Settings->WorldSpace)
                    tmp = rotMat * vn;

                ObjFile << "vn " << tmp.x << " " << tmp.y << " " << tmp.z << std::endl;
            }

            for (auto &&vt : mesh->UVs)
                ObjFile << "vt " << vt.x << " " << vt.y << std::endl;

            for (auto &&f : mesh->Faces)
            {
                size_t matID = 0;
                auto IT = processedMaterials.find(f->MaterialIndex);

                if(IT == processedMaterials.end())
                {
                    processedMaterials.insert({f->MaterialIndex, MatCounter});
                    matID = MatCounter;

                    float Ambient = 1.0;
                    int Illum = 2;
                    float Transparency = 0;
                    float Alpha = 1.0;

                    if(f->FaceMaterial->Metallic != 0.0)
                    {
                        Ambient = f->FaceMaterial->Metallic;
                        Illum = 3;
                    }
                    else if(f->FaceMaterial->Transparency != 0.0) // Glass
                    {
                        Illum = 4;
                        Transparency = f->FaceMaterial->Transparency;
                        Alpha = 1 - f->FaceMaterial->Transparency;
                    }

                    MTLFile << "newmtl Mat" << MatCounter << std::endl;
                    MTLFile << "Ns " << f->FaceMaterial->Roughness * 1000.f << std::endl;
                    MTLFile << "Ka " << Ambient << " " << Ambient << " " << Ambient << std::endl;
                    MTLFile << "Kd 1.0 1.0 1.0" << std::endl;
                    MTLFile << "Ks " << f->FaceMaterial->Specular << " " << f->FaceMaterial->Specular << " " << f->FaceMaterial->Specular << std::endl;
                    
                    if(f->FaceMaterial->Power != 0.0)
                    {
                        MTLFile << "Ke " << f->FaceMaterial->Power << " " << f->FaceMaterial->Power << " " << f->FaceMaterial->Power << std::endl;
                        MTLFile << "map_Ke " << m_ExternalFilenames << ".emission.png" << std::endl;
                    }

                    MTLFile << "Tr " << Transparency << std::endl;
                    MTLFile << "d " << Alpha << std::endl;
                    MTLFile << "Ni " << f->FaceMaterial->IOR << std::endl;
                    MTLFile << "illum " << Illum << std::endl;
                    MTLFile << "map_Kd " << m_ExternalFilenames << ".albedo.png" << std::endl;


                    MatCounter++;  
                }
                else
                    matID = IT->second;
                
                ObjFile << "usemtl Mat" << matID << std::endl;

                for (size_t i = 0; i < f->Indices.size(); i += 3)
                {
                    ObjFile << "f";
                    for (char j = 0; j < 3; j++)
                    {
                        Math::Vec3f tmp = f->Indices[i + j] + indicesOffset;

                        ObjFile << " ";
                        ObjFile << tmp.x << "/" << tmp.z << "/" << tmp.y;
                    }
                    ObjFile << std::endl;
                }       
            }
        
            indicesOffset += Math::Vec3f(mesh->Vertices.size(), mesh->Normals.size(), mesh->UVs.size());
        }

        std::string ObjFileStr = ObjFile.str();
        std::string MTLFileStr = MTLFile.str();

        auto textures = Meshes[0]->Textures;

        std::map<std::string, std::vector<char>> ret = 
        {
            {"obj", std::vector<char>(ObjFileStr.begin(), ObjFileStr.end())},
            {"mtl", std::vector<char>(MTLFileStr.begin(), MTLFileStr.end())},
            {"albedo.png", textures[TextureType::DIFFIUSE]->AsPNG()}
        };

        if(textures.find(TextureType::EMISSION) != textures.end())
            ret["emission.png"] = textures[TextureType::EMISSION]->AsPNG();

        return ret;
    }
}
