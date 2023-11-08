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

#include <sstream>
#include "PLYExporter.hpp"

namespace VCore
{
    std::map<std::string, std::vector<char>> CPLYExporter::Generate(std::vector<Mesh> Meshes)
    {
        size_t counter = 0;
        std::map<std::string, std::vector<char>> ret;

        for (auto &&mesh : Meshes)
        {
            std::stringstream vertexList, faceList;
            size_t vertexCount = 0, faceCount = 0, indexOffset = 0;

            //"Extracts" the rotation matrix. Quick'n Dirty
            Math::Mat4x4 rotMat = mesh->ModelMatrix;
            rotMat.x.w = 0;
            rotMat.y.w = 0;
            rotMat.z.w = 0;

            for (auto &&surface : mesh->Surfaces)
            {
                for(int i = 0; i < surface.Size(); i++)
                {
                    auto v = surface[i];
                    vertexList << v.Pos.x << " " << v.Pos.z << " " << v.Pos.y << " " << v.Normal.x << " " << v.Normal.z << " " << v.Normal.y << " " << v.UV.x << " " << v.UV.y << std::endl;
                    vertexCount++;
                }
                
                for (size_t i = 0; i < surface.Indices.size(); i += 3)
                {
                    faceList << "3 " << surface.Indices[i] + indexOffset << " " << surface.Indices[i + 1] + indexOffset << " " << surface.Indices[i + 2] + indexOffset << std::endl;
                    faceCount++;
                }
                indexOffset += surface.Indices.size();
            }

            // Fileheader
            std::stringstream file;
            file << "ply" << std::endl;
            file << "format ascii 1.0" << std::endl;
            file << "comment Generated with VCore" << std::endl;
            file << "element vertex " << vertexCount << std::endl;
            file << "property float x" << std::endl;
            file << "property float y" << std::endl;
            file << "property float z" << std::endl;
            file << "property float nx" << std::endl;
            file << "property float nx" << std::endl;
            file << "property float nz" << std::endl;
            file << "property float s" << std::endl;
            file << "property float t" << std::endl;
            file << "element face " << faceCount << std::endl;
            file << "property list uchar uint vertex_indices" << std::endl;
            file << "end_header" << std::endl;

            file << vertexList.rdbuf() << faceList.rdbuf();
                                    
            std::string fileStr = file.str();

            std::string ext = "ply";
            if(Meshes.size() > 1)
                ext = std::to_string(counter) + ".ply";

            ret.insert({ext, std::vector<char>(fileStr.begin(), fileStr.end())});
            counter++;
        }

        return ret;
    }
}
