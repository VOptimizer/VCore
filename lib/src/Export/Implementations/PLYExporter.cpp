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
#include "../../FileUtils.hpp"

namespace VCore
{
    void CPLYExporter::WriteData(const std::string &_Path, const std::vector<Mesh> &_Meshes)
    {
        size_t counter = 0;
        auto pathWithoutExt = GetPathWithoutExt(_Path);

        for (auto &&mesh : _Meshes)
        {
            auto filestrm = m_IOHandler->Open(pathWithoutExt + "." + std::to_string(counter) + ".ply", "wb");

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
                indexOffset += surface.Size();
            }

            // Fileheader
            filestrm->Write("ply\n");
            filestrm->Write("format ascii 1.0\n");
            filestrm->Write("comment Generated with VCore (https://github.com/VOptimizer/VCore)\n");
            filestrm->Write("element vertex " + std::to_string(vertexCount) + "\n");
            filestrm->Write("property float x\n");
            filestrm->Write("property float y\n");
            filestrm->Write("property float z\n");
            filestrm->Write("property float nx\n");
            filestrm->Write("property float nx\n");
            filestrm->Write("property float nz\n");
            filestrm->Write("property float s\n");
            filestrm->Write("property float t\n");
            filestrm->Write("element face " + std::to_string(faceCount) + "\n");
            filestrm->Write("property list uchar uint vertex_indices\n");
            filestrm->Write("end_header\n");

            filestrm->Write(vertexList.str() + "\n");
            filestrm->Write(faceList.str() + "\n");

            counter++;
            m_IOHandler->Close(filestrm);
        }
    }
}
