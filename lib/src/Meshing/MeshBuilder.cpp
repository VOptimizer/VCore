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

#include <VCore/Meshing/Mesh/MeshBuilder.hpp>
#include <VCore/Misc/Exceptions.hpp>

namespace VCore
{
    void CMeshBuilder::AddTextures(const ankerl::unordered_dense::map<TextureType, Texture> &p_Textures)
    {
        m_Textures = &p_Textures;
    }

    int CMeshBuilder::AddVertex(const SVertex &p_Vertex, SIndexedSurface &p_Surface)
    {
        // TODO: How to index for simple mesher. Greedy not neccessary because of "random" distribution
        // FIXME: The old me is stupid.
        auto it = p_Surface.Index.find(p_Vertex);
        if(it == p_Surface.Index.end())
        {
            int idx = p_Surface.Surface->GetVertexCount();
            // TODO:
            // _Surface.Surface->AddVertex(_Vertex);
            p_Surface.Index.insert({p_Vertex, idx});
            return idx;
        }

        return it->second;
    }

    Mesh CMeshBuilder::Build()
    {
        auto ret = std::make_shared<SMesh>();
        for (auto &&surface : m_Surfaces)
            ret->Surfaces.push_back(std::move(surface.second.Surface));
        
        // Clears the cache.
        m_Surfaces.clear();

        return ret;
    }

    void CMeshBuilder::SelectSurface(const uint8_t p_MaterialHandle)
    {
        auto it = m_Surfaces.find(p_MaterialHandle);
        if(it == m_Surfaces.end())
        {
            SIndexedSurface surface(m_SurfaceFactory());
            surface.Surface->MaterialHandle = p_MaterialHandle;
            it = m_Surfaces.insert({p_MaterialHandle, surface}).first;
        }

        m_CurrentSurface = it->second.Surface;
    }

    uint32_t CMeshBuilder::AddVertex(const SVertex* p_Vertex)
    {
        return m_CurrentSurface->AddVertex(p_Vertex);
    }

    void CMeshBuilder::AddFace(uint32_t p_Idx1, uint32_t p_Idx2, uint32_t p_Idx3, uint32_t p_Idx4)
    {
        m_CurrentSurface->AddFace(p_Idx1, p_Idx2, p_Idx3);
        m_CurrentSurface->AddFace(p_Idx2, p_Idx4, p_Idx3);
    }

    Mesh CMeshBuilder::Merge(Mesh p_MergeInto, const std::vector<Mesh> &p_Meshes, bool p_ApplyModelMatrix)
    {
        Mesh ret;
        if(p_MergeInto)
        {
            GenerateCache(p_MergeInto);
            ret = p_MergeInto;
        }
        else
        {
            ret = std::make_shared<SMesh>();
            if(!p_Meshes.empty())
                ret->Textures = p_Meshes[0]->Textures;
        }

        // for (auto &&m : _Meshes)
        // {
        //     for(auto &&surface : m->Surfaces)
        //     {
        //         auto it = m_Surfaces.find((size_t)surface.FaceMaterial);
        //         if(it == m_Surfaces.end())
        //             it = m_Surfaces.insert({(size_t)surface.FaceMaterial, SIndexedSurface(surface.FaceMaterial)}).first;

        //         it->second.Surface.Vertices.reserve(it->second.Surface.Vertices.capacity() + surface.Vertices.size());
        //         it->second.Surface.Indices.reserve(it->second.Surface.Indices.capacity() + surface.Indices.size());
        //     }

        // }

        for (auto &&m : p_Meshes)       
            MergeIntoThis(m, p_ApplyModelMatrix);

        ret->Surfaces.clear();
        for (auto &&surface : m_Surfaces)
            ret->Surfaces.push_back(std::move(surface.second.Surface));

        // Clears the cache.
        m_Surfaces.clear();

        return ret;
    }

    bool CMeshBuilder::IsOnBorder(const Math::Vec3f &p_Pos)
    {
        for (size_t i = 0; i < 3; i++)
        {
            int pos = p_Pos.v[i] - ((int)(p_Pos.v[i] / (float)Config::ChunkSize) * Config::ChunkSize);

            // TODO: Should I ever make the chunk size dynamically, than must this be also dynamic.
            if(pos == 0 || pos == Config::InnerChunkMask)
                return true;
        }

        return false;
    }

    void CMeshBuilder::GenerateCache(Mesh p_MergeInto)
    {
        m_Textures = &p_MergeInto->Textures;

        for (auto &&surface : p_MergeInto->Surfaces)
        {
            auto it = m_Surfaces.find(surface->MaterialHandle);
            if(it == m_Surfaces.end())
                it = m_Surfaces.insert({surface->MaterialHandle, SIndexedSurface(nullptr)}).first;
            
            it->second.Surface = std::move(surface);
            for (uint64_t i = 0; i < it->second.Surface->GetVertexCount(); i++)
            {
                auto v = it->second.Surface->GetVertex(i);
                if(IsOnBorder(v.Pos))
                    it->second.Index.insert({v, i});
                //     AddVertex(v, it->second);
                // else
                //     it->second.Surface.AddVertex(v);
            }
        }       
    }

    uint32_t CMeshBuilder::AddMergeVertex(const SVertex &p_Vertex, SIndexedSurface &p_Surface, ankerl::unordered_dense::map<SVertex, int, VertexHasher> &p_Index)
    {
        int idx;
        if(IsOnBorder(p_Vertex.Pos))
            idx = AddVertex(p_Vertex, p_Surface);
        else
        {
            auto it = p_Index.find(p_Vertex);
            if(it != p_Index.end())
                idx = it->second;
            else
            {
                idx = p_Surface.Surface->GetVertexCount();
                // TODO:
                // _Surface.Surface->AddVertex(_Vertex);
                p_Index.insert({p_Vertex, idx});
            }
        }

        return idx;
    }

    void CMeshBuilder::MergeIntoThis(Mesh p_Mesh, bool p_ApplyModelMatrix)
    {
        Math::Mat4x4 rotation;
        static ankerl::unordered_dense::map<SVertex, int, VertexHasher> localIndex;

        if(p_ApplyModelMatrix)
        {
            auto euler = p_Mesh->ModelMatrix.GetEuler();
            rotation
                .Rotate(Math::Vec3f(0, 0, 1), euler.z)
                .Rotate(Math::Vec3f(1, 0, 0), euler.x)
                .Rotate(Math::Vec3f(0, 1, 0), euler.y);
        }

        for (auto &&surface : p_Mesh->Surfaces)
        {
            auto it = m_Surfaces.find(surface->MaterialHandle);
            if(it == m_Surfaces.end())
            {
                auto newSurface = m_SurfaceFactory();
                newSurface->MaterialHandle = surface->MaterialHandle;

                it = m_Surfaces.insert({surface->MaterialHandle, SIndexedSurface(newSurface)}).first;
            }

            if(!p_ApplyModelMatrix)
                it->second.Surface->MergeSurface(surface);
            else
            {
                // TODO: SLOW
                it->second.Surface->ReserveVertices(it->second.Surface->GetVertexCount() + surface->GetVertexCount());
                it->second.Surface->ReserveFaces(it->second.Surface->GetFaceCount() + surface->GetFaceCount());
                for (uint64_t i = 0; i < surface->GetFaceCount(); i++)
                {
                    SVertex v1 = surface->GetVertex(surface->GetIndex(i * 3));
                    SVertex v2 = surface->GetVertex(surface->GetIndex(i * 3 + 1));
                    SVertex v3 = surface->GetVertex(surface->GetIndex(i * 3) + 2);

                    if(p_ApplyModelMatrix)
                    {
                        v1.Pos = p_Mesh->ModelMatrix * v1.Pos;
                        v1.Normal = rotation * v1.Normal;

                        v2.Pos = p_Mesh->ModelMatrix * v2.Pos;
                        v2.Normal = rotation * v2.Normal;

                        v3.Pos = p_Mesh->ModelMatrix * v3.Pos;
                        v3.Normal = rotation * v3.Normal;
                    }
                    AddMergeVertex(v1, it->second, localIndex);
                    AddMergeVertex(v2, it->second, localIndex);
                    AddMergeVertex(v3, it->second, localIndex);
                }
            }
        }

        localIndex.clear();
    }
}
