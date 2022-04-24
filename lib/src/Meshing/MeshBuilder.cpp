/*
 * MIT License
 *
 * Copyright (c) 2022 Christian Tost
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

#include <VoxelOptimizer/Meshing/MeshBuilder.hpp>
#include <VoxelOptimizer/Misc/Exceptions.hpp>

namespace VoxelOptimizer
{
    CMeshBuilder::CMeshBuilder() : m_CurrentMesh(new SMesh()) { }

    void CMeshBuilder::AddTextures(const std::map<TextureType, Texture> &_textures)
    {
        m_CurrentMesh->Textures = _textures;
    }

    void CMeshBuilder::AddFace(CVector _v1, CVector _v2, CVector _v3, CVector _v4, CVector _normal, int _color, Material _material)
    {
        if(m_CurrentMesh->Textures.empty())
            throw CMeshBuilderException("Please call first CMeshBuilder::AddTextures in order to generate an uv map!");

        int i1, i2, i3, i4;
        i1 = AddPosition(_v1);
        i2 = AddPosition(_v2);
        i3 = AddPosition(_v3);
        i4 = AddPosition(_v4);

        GroupedFaces faces;
        auto itFaces = m_FacesIndex.find(_material);
        if(itFaces == m_FacesIndex.end())
        {
            // Adds a new face group, if _material is a new material.
            faces = std::make_shared<SGroupedFaces>();
            m_CurrentMesh->Faces.push_back(faces);

            faces->FaceMaterial = _material;
            m_FacesIndex.insert({_material, faces});
        }
        else
            faces = itFaces->second;
            
        CVector faceNormal = (_v2 - _v1).Cross(_v3 - _v1).Normalize(); 
        int normalIdx = AddNormal(_normal);
        int uvIdx = AddUV(CVector(((float)(_color + 0.5f)) / m_CurrentMesh->Textures[TextureType::DIFFIUSE]->Size().x, 0.5f, 0));

        // Checks the direction of the face.
        if(faceNormal == _normal)
        {
            faces->Indices.push_back(CVector(i1, normalIdx, uvIdx));
            faces->Indices.push_back(CVector(i2, normalIdx, uvIdx));
            faces->Indices.push_back(CVector(i3, normalIdx, uvIdx));

            faces->Indices.push_back(CVector(i1, normalIdx, uvIdx));
            faces->Indices.push_back(CVector(i3, normalIdx, uvIdx));
            faces->Indices.push_back(CVector(i4, normalIdx, uvIdx));
        }
        else
        {
            faces->Indices.push_back(CVector(i3, normalIdx, uvIdx));
            faces->Indices.push_back(CVector(i2, normalIdx, uvIdx));
            faces->Indices.push_back(CVector(i1, normalIdx, uvIdx));

            faces->Indices.push_back(CVector(i4, normalIdx, uvIdx));
            faces->Indices.push_back(CVector(i3, normalIdx, uvIdx));
            faces->Indices.push_back(CVector(i1, normalIdx, uvIdx));
        }
    }
   
    void CMeshBuilder::AddFace(SVertex v1, SVertex v2, SVertex v3)
    {
        int I1, I2, I3;
        I1 = AddPosition(v1.Pos);
        I2 = AddPosition(v2.Pos);
        I3 = AddPosition(v3.Pos);

        GroupedFaces Faces;

        auto ITFaces = m_FacesIndex.find(v1.Mat);
        if(ITFaces == m_FacesIndex.end())
        {
            Faces = std::make_shared<SGroupedFaces>();
            m_CurrentMesh->Faces.push_back(Faces);

            Faces->FaceMaterial = v1.Mat;
            m_FacesIndex.insert({v1.Mat, Faces});
        }
        else
            Faces = ITFaces->second;
            
        // CVector FaceNormal = (v2 - v1).Cross(v3 - v1).Normalize(); 
        int NormalIdx = AddNormal(v1.Normal);
        int UVIdx1 = AddUV(v1.UV);
        int UVIdx2 = AddUV(v2.UV);
        int UVIdx3 = AddUV(v3.UV);

        Faces->Indices.push_back(CVector(I1, NormalIdx, UVIdx1));
        Faces->Indices.push_back(CVector(I2, NormalIdx, UVIdx2));
        Faces->Indices.push_back(CVector(I3, NormalIdx, UVIdx3));
    }

    int CMeshBuilder::AddPosition(CVector _pos)
    {
        int Ret = 0;
        auto IT = m_Index.find(_pos);

        if(IT != m_Index.end())
            Ret = IT->second;
        else
        {
            m_CurrentMesh->Vertices.push_back(_pos);
            Ret = m_CurrentMesh->Vertices.size();

            m_Index.insert({_pos, Ret});
        }

        return Ret;
    }

    int CMeshBuilder::AddNormal(CVector _normal)
    {
        int Ret = 0;
        auto IT = m_NormalIndex.find(_normal);

        if(IT != m_NormalIndex.end())
            Ret = IT->second;
        else
        {
            m_CurrentMesh->Normals.push_back(_normal);
            Ret = m_CurrentMesh->Normals.size();

            m_NormalIndex.insert({_normal, Ret});
        }

        return Ret;
    }

    int CMeshBuilder::AddUV(CVector _uv)
    {
        int Ret = 0;
        auto IT = m_UVIndex.find(_uv);

        if(IT != m_UVIndex.end())
            Ret = IT->second;
        else
        {
            m_CurrentMesh->UVs.push_back(_uv);
            Ret = m_CurrentMesh->UVs.size();

            m_UVIndex.insert({_uv, Ret});
        }

        return Ret;
    }

    Mesh CMeshBuilder::Build()
    {
        // Clears the cache.
        m_Index.clear();
        m_NormalIndex.clear();
        m_UVIndex.clear();
        m_FacesIndex.clear();

        auto ret = m_CurrentMesh;
        m_CurrentMesh = std::make_shared<SMesh>();

        return ret;
    }

    Mesh CMeshBuilder::Copy(Mesh mesh)
    {
        auto ret = std::make_shared<SMesh>();

        ret->Vertices = mesh->Vertices;
        ret->UVs = mesh->UVs;
        ret->Normals = mesh->Normals;

        ret->ModelMatrix = mesh->ModelMatrix;
        ret->Textures = mesh->Textures;

        for (auto &&f : mesh->Faces)
        {
            auto face = std::make_shared<SGroupedFaces>();
            face->FaceMaterial = f->FaceMaterial;
            face->Indices = f->Indices;
            face->MaterialIndex = f->MaterialIndex;

            ret->Faces.push_back(face);
        }

        return ret;
    }

    void CMeshBuilder::Merge(Mesh mergeInto, const std::vector<Mesh> &meshes)
    {
        m_CurrentMesh = mergeInto;
        GenerateCache();

        for (auto &&m : meshes)       
            MergeIntoThis(m);
    }

    void CMeshBuilder::GenerateCache()
    {
        for (auto &&f : m_CurrentMesh->Faces)
            m_FacesIndex.insert({f->FaceMaterial, f});

        for (size_t i = 0; i < m_CurrentMesh->Vertices.size(); i++)
            m_Index.insert({m_CurrentMesh->Vertices[i], i + 1});      

        for (size_t i = 0; i < m_CurrentMesh->UVs.size(); i++)
            m_UVIndex.insert({m_CurrentMesh->UVs[i], i + 1});  

        for (size_t i = 0; i < m_CurrentMesh->Normals.size(); i++)
            m_NormalIndex.insert({m_CurrentMesh->Normals[i], i + 1});            
    }

    void CMeshBuilder::MergeIntoThis(Mesh m)
    {
        auto euler = m->ModelMatrix.GetEuler();
        CMat4x4 rotation;
        rotation
            .Rotate(CVector(0, 0, 1), euler.z)
            .Rotate(CVector(1, 0, 0), euler.x)
            .Rotate(CVector(0, 1, 0), euler.y);
            
        for (auto &&f : m->Faces)
        {
            for (int i = 0; i < f->Indices.size(); i += 3)
            {
                CVector idx1 = f->Indices[i];
                CVector idx2 = f->Indices[i + 1];
                CVector idx3 = f->Indices[i + 2];

                SVertex v1 = {
                    m->ModelMatrix * m->Vertices[idx1.x - 1],
                    m->UVs[idx1.z - 1],
                    rotation * m->Normals[idx1.y - 1],
                    f->FaceMaterial,
                    f->MaterialIndex
                };

                SVertex v2 = {
                    m->ModelMatrix * m->Vertices[idx2.x - 1],
                    m->UVs[idx2.z - 1],
                    rotation * m->Normals[idx2.y - 1],
                    f->FaceMaterial,
                    f->MaterialIndex
                };

                SVertex v3 = {
                    m->ModelMatrix * m->Vertices[idx3.x - 1],
                    m->UVs[idx3.z - 1],
                    rotation * m->Normals[idx3.y - 1],
                    f->FaceMaterial,
                    f->MaterialIndex
                };

                AddFace(v1, v2, v3);
            }
        }
    }
} // namespace VoxelOptimizer
