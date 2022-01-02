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

#include <VoxelOptimizer/Meshers/MeshBuilder.hpp>

namespace VoxelOptimizer
{
    CMeshBuilder::CMeshBuilder()
    {
        m_CurrentMesh = Mesh(new SMesh());
    }

    void CMeshBuilder::AddFace(CVector v1, CVector v2, CVector v3, CVector v4, CVector Normal, int Color, int Material)
    {
        int I1, I2, I3, I4;
        I1 = AddVertex(v1);
        I2 = AddVertex(v2);
        I3 = AddVertex(v3);
        I4 = AddVertex(v4);

        GroupedFaces Faces;

        auto ITFaces = m_FacesIndex.find(Material);
        if(ITFaces == m_FacesIndex.end())
        {
            Faces = GroupedFaces(new SGroupedFaces());
            m_CurrentMesh->Faces.push_back(Faces);

            Faces->MaterialIndex = Material;
            Faces->FaceMaterial = m_Loader->GetMaterials()[Material];
            m_FacesIndex.insert({Material, Faces});
        }
        else
            Faces = ITFaces->second;
            
        CVector FaceNormal = (v2 - v1).Cross(v3 - v1).Normalize(); 
        int NormalIdx = AddNormal(Normal);
        int UVIdx = AddUV(CVector(((float)(Color + 0.5f)) / m_CurrentMesh->Textures[TextureType::DIFFIUSE]->Size().x, 0.5f, 0));

        if(FaceNormal == Normal)
        {
            Faces->Indices.push_back(CVector(I1, NormalIdx, UVIdx));
            Faces->Indices.push_back(CVector(I2, NormalIdx, UVIdx));
            Faces->Indices.push_back(CVector(I3, NormalIdx, UVIdx));

            Faces->Indices.push_back(CVector(I1, NormalIdx, UVIdx));
            Faces->Indices.push_back(CVector(I3, NormalIdx, UVIdx));
            Faces->Indices.push_back(CVector(I4, NormalIdx, UVIdx));
        }
        else
        {
            Faces->Indices.push_back(CVector(I3, NormalIdx, UVIdx));
            Faces->Indices.push_back(CVector(I2, NormalIdx, UVIdx));
            Faces->Indices.push_back(CVector(I1, NormalIdx, UVIdx));

            Faces->Indices.push_back(CVector(I4, NormalIdx, UVIdx));
            Faces->Indices.push_back(CVector(I3, NormalIdx, UVIdx));
            Faces->Indices.push_back(CVector(I1, NormalIdx, UVIdx));
        }
    }
   
    void CMeshBuilder::AddFace(SVertex v1, SVertex v2, SVertex v3)
    {
        int I1, I2, I3;
        I1 = AddVertex(v1.Pos);
        I2 = AddVertex(v2.Pos);
        I3 = AddVertex(v3.Pos);

        GroupedFaces Faces;

        auto ITFaces = m_FacesIndex.find(v1.Material);
        if(ITFaces == m_FacesIndex.end())
        {
            Faces = GroupedFaces(new SGroupedFaces());
            m_CurrentMesh->Faces.push_back(Faces);

            Faces->MaterialIndex = v1.Material;
            Faces->FaceMaterial = m_Loader->GetMaterials()[v1.Material];
            m_FacesIndex.insert({v1.Material, Faces});
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

    int CMeshBuilder::AddVertex(CVector Vertex)
    {
        int Ret = 0;
        // auto End = m_CurrentMesh->Vertices.data() + m_CurrentMesh->Vertices.size();
        auto IT = m_Index.find(Vertex); //std::find(m_CurrentMesh->Vertices.data(), End, Vertex);

        if(IT != m_Index.end())
            Ret = IT->second;
        else
        {
            m_CurrentMesh->Vertices.push_back(Vertex);
            Ret = m_CurrentMesh->Vertices.size();

            m_Index.insert({Vertex, Ret});
        }

        return Ret;
    }

    int CMeshBuilder::AddNormal(CVector Normal)
    {
        int Ret = 0;
        // Normal = CVector(Normal.x, Normal.z, Normal.y);

        // auto End = m_CurrentMesh->Vertices.data() + m_CurrentMesh->Vertices.size();
        auto IT = m_NormalIndex.find(Normal); //std::find(m_CurrentMesh->Vertices.data(), End, Vertex);

        if(IT != m_NormalIndex.end())
            Ret = IT->second;
        else
        {
            m_CurrentMesh->Normals.push_back(Normal);
            Ret = m_CurrentMesh->Normals.size();

            m_NormalIndex.insert({Normal, Ret});
        }

        return Ret;
    }

    int CMeshBuilder::AddUV(CVector UV)
    {
        int Ret = 0;
        // Normal = CVector(Normal.x, Normal.z, Normal.y);

        // auto End = m_CurrentMesh->Vertices.data() + m_CurrentMesh->Vertices.size();
        auto IT = m_UVIndex.find(UV); //std::find(m_CurrentMesh->Vertices.data(), End, Vertex);

        if(IT != m_UVIndex.end())
            Ret = IT->second;
        else
        {
            m_CurrentMesh->UVs.push_back(UV);
            Ret = m_CurrentMesh->UVs.size();

            m_UVIndex.insert({UV, Ret});
        }

        return Ret;
    }

    Mesh CMeshBuilder::Build()
    {
        m_Index.clear();
        m_NormalIndex.clear();
        m_UVIndex.clear();
        m_FacesIndex.clear();

        auto ret = m_CurrentMesh;
        m_CurrentMesh = Mesh(new SMesh());

        return ret;
    }

    Mesh CMeshBuilder::Copy(Mesh mesh)
    {
        auto ret = Mesh(new SMesh());

        ret->Vertices = mesh->Vertices;
        ret->UVs = mesh->UVs;
        ret->Normals = mesh->Normals;

        ret->ModelMatrix = mesh->ModelMatrix;
        ret->Textures = mesh->Textures;

        for (auto &&f : mesh->Faces)
        {
            auto face = GroupedFaces(new SGroupedFaces());
            face->FaceMaterial = f->FaceMaterial;
            face->Indices = f->Indices;
            face->MaterialIndex = f->MaterialIndex;

            ret->Faces.push_back(face);
        }

        return ret;
    }

    void CMeshBuilder::Merge(const std::vector<Mesh> &meshes)
    {
        m_CurrentMesh->Textures[TextureType::DIFFIUSE] = meshes.front()->Textures[TextureType::DIFFIUSE];

        if(meshes.front()->Textures.find(TextureType::EMISSION) != meshes.front()->Textures.end())
            m_CurrentMesh->Textures[TextureType::EMISSION] = meshes.front()->Textures[TextureType::EMISSION];

        for (auto &&m : meshes)
        {            
            auto rotation = m->ModelMatrix;
            
            for (auto &&f : m->Faces)
            {
                for (int i = 0; i < f->Indices.size(); i += 3)
                {
                    CVector idx1 = f->Indices[i];
                    CVector idx2 = f->Indices[i + 1];
                    CVector idx3 = f->Indices[i + 2];
                    

                    SVertex v1 = {
                        m->ModelMatrix * m->Vertices[idx1.x],
                        m->UVs[idx1.z],
                        rotation * m->Normals[idx1.y],
                        f->MaterialIndex
                    };

                    SVertex v2 = {
                        m->ModelMatrix * m->Vertices[idx2.x],
                        m->UVs[idx2.z],
                        rotation * m->Normals[idx2.y],
                        f->MaterialIndex
                    };

                    SVertex v3 = {
                        m->ModelMatrix * m->Vertices[idx3.x],
                        m->UVs[idx3.z],
                        rotation * m->Normals[idx3.y],
                        f->MaterialIndex
                    };

                    AddFace(v1, v2, v3);
                }
            }
        }
    }
} // namespace VoxelOptimizer
