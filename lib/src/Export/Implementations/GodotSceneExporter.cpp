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
#include "GodotSceneExporter.hpp"
#include "../../FileUtils.hpp"

namespace VCore
{
    void CGodotSceneExporter::WriteData(const std::string &_Path, const std::vector<Mesh> &_Meshes)
    {
        auto filenameWithoutExt = GetFilenameWithoutExt(_Path);
        std::stringstream os, nodes;
        // os << "[gd_scene load_steps=" << 3 + Mesh->Faces.size() << " format=2]\n" << std::endl;
        size_t id = 1;
        os << "[ext_resource path=\"res://" << filenameWithoutExt << ".albedo.png\" type=\"Texture\" id=1]\n" << std::endl;

        auto textures = _Meshes[0]->Textures;
        if(textures.find(TextureType::EMISSION) != textures.end())
        {
            os << "[ext_resource path=\"res://" << filenameWithoutExt << ".emission.png\" type=\"Texture\" id=2]\n" << std::endl;
            id++;
        }

        size_t loadStepsSize = 0;

        nodes << "[node name=\"root\" type=\"Spatial\"]\n" << std::endl;

        for (auto &&mesh : _Meshes)
        {
            std::stringstream arrayMesh;
            loadStepsSize += mesh->Surfaces.size();

            size_t surfaceIdx = 0;
            for (auto &&surface : mesh->Surfaces)
            {
                os << "[sub_resource type=\"SpatialMaterial\" id=" << id << "]" << std::endl;
                os << "albedo_texture = ExtResource( 1 )" << std::endl;
                os << "metallic = " << surface.FaceMaterial->Metallic << std::endl;
                os << "metallic_specular = " << surface.FaceMaterial->Specular << std::endl;
                os << "roughness = " << surface.FaceMaterial->Roughness << std::endl;

                if(surface.FaceMaterial->Power != 0)
                {
                    os << "emission_enabled = true" << std::endl;
                    os << "emission_energy = " << surface.FaceMaterial->Power << std::endl;
                    os << "emission_texture = ExtResource( 2 )" << std::endl;
                }

                if(surface.FaceMaterial->IOR != 0)
                {
                    os << "refraction_enabled = true" << std::endl;
                    os << "refraction_energy = " << surface.FaceMaterial->IOR << std::endl;
                }

                if(surface.FaceMaterial->Transparency != 0.0)
                {
                    os << "flags_transparent = true" << std::endl;
                    os << "albedo_color = Color( 1, 1, 1, " << 1.f - surface.FaceMaterial->Transparency << ")" << std::endl;
                }

                arrayMesh << "surfaces/" << surfaceIdx << "= {" << std::endl;
                arrayMesh << "\t\"material\":SubResource(" << id << ")," << std::endl;
                arrayMesh << "\t\"primitive\":4," << std::endl;
                arrayMesh << "\t\"arrays\":[" << std::endl;
                surfaceIdx++;

                // Lazy memory management.
                {
                    std::stringstream vertexList, normalList, uvList;
                    vertexList << "\t\tVector3Array(";
                    normalList << "\t\tVector3Array(";
                    uvList << "\t\tVector2Array(";
                    bool first = true;
                    for(int i = 0; i < surface.Size(); i++)
                    {
                        auto v = surface[i];

                        if(!first)
                        {
                            vertexList << ", ";
                            normalList << ", ";
                            uvList << ", ";
                        }

                        vertexList << v.Pos.x << ", " << v.Pos.y << ", " << v.Pos.z;
                        normalList << v.Normal.x << ", " << v.Normal.y << ", " << v.Normal.z;
                        uvList << v.UV.x << ", " << v.UV.y;
                        first = false;
                    }
                    vertexList << ")," << std::endl;
                    normalList << ")," << std::endl;
                    uvList << ")," << std::endl;

                    arrayMesh << vertexList.rdbuf();
                    arrayMesh << normalList.rdbuf();

                    arrayMesh << "\t\tnull," << std::endl;
                    arrayMesh << "\t\tnull," << std::endl;

                    arrayMesh << uvList.rdbuf();
                    arrayMesh << "\t\tnull," << std::endl;
                    arrayMesh << "\t\tnull," << std::endl;
                    arrayMesh << "\t\tnull," << std::endl;
                }

                arrayMesh << "\t\tIntArray(";
                bool first = true;
                
                // Godot uses Clockwise Winding Order for culling.
                for (size_t i = 0; i < surface.Indices.size(); i += 3)                
                {
                    if(!first)
                        arrayMesh << ", ";

                    arrayMesh << surface.Indices[i] << ", " << surface.Indices[i + 2] << ", " << surface.Indices[i + 1];
                    first = false;
                }
                arrayMesh << ")" << std::endl;
                arrayMesh << "\t]," << std::endl;

                arrayMesh << "\t\"morph_arrays\":[]" << std::endl;
                arrayMesh << "}" << std::endl;

                id++;
            }        
    
            os << "\n[sub_resource id=" << id << " type=\"ArrayMesh\"]\n" << std::endl;
            os << arrayMesh.str() << std::endl;

            nodes << "[node name=\"Voxel" << id << "\" type=\"MeshInstance\" parent=\".\"]\n" << std::endl;
            nodes << "mesh = SubResource(" << id << ")" << std::endl;
            nodes << "visible = true" << std::endl;

            if(m_Settings->WorldSpace)
            {
                auto matrix = mesh->ModelMatrix;

                nodes << "transform = Transform(";
                nodes << matrix.x.x << ", " << matrix.y.x << ", " << matrix.z.x << ", ";
                nodes << matrix.x.y << ", " << matrix.y.y << ", " << matrix.z.y << ", ";
                nodes << matrix.x.z << ", " << matrix.y.z << ", " << matrix.z.z << ", ";
                nodes << matrix.x.w << ", " << matrix.y.w << ", " << matrix.z.w << ")" << std::endl;
            }
            else
                nodes << "transform = Transform(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0)" << std::endl;
        }

        os << nodes.str();
        
        std::string ESCNFileStr = "[gd_scene load_steps=" + std::to_string(3 + loadStepsSize) + " format=2]\n\n" + os.str();

        auto strm = m_IOHandler->Open(_Path, "wb");
        strm->Write(ESCNFileStr);
        m_IOHandler->Close(strm);

        SaveTexture(textures[TextureType::DIFFIUSE], _Path, "albedo");
        if(textures.find(TextureType::EMISSION) != textures.end())
            SaveTexture(textures[TextureType::EMISSION], _Path, "emission");
    }
}
