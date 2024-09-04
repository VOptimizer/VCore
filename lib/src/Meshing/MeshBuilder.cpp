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
    void CMeshBuilder::AddTextures(const ankerl::unordered_dense::map<TextureType, Texture> &_textures)
    {
        m_Textures = &_textures;
    }

    int CMeshBuilder::AddVertex(const SVertex &_Vertex, SIndexedSurface &_Surface)
    {
        auto cell = Math::Vec3i(_Vertex.Pos / (CHUNK_SIZE + 1));
        auto it = _Surface.Index2.find(cell);
        if(it == _Surface.Index2.end())
            it = _Surface.Index2.insert({cell, {}}).first;

        auto it2 = it->second.find(_Vertex);
        if(it2 == it->second.end())
        {
            int idx = _Surface.Surface->GetVertexCount();
            _Surface.Surface->AddVertex(_Vertex);
            it->second.insert({_Vertex, idx});
            // _Surface.Index.insert({_Vertex, idx});
            return idx;
        }

        return it2->second;

        // // TODO: How to index for simple mesher. Greedy not neccessary because of "random" distribution
        // // FIXME: The old me is stupid.
        // auto it = _Surface.Index.find(_Vertex);
        // if(it == _Surface.Index.end())
        // {
        //     int idx = _Surface.Surface->GetVertexCount();
        //     _Surface.Surface->AddVertex(_Vertex);
        //     _Surface.Index.insert({_Vertex, idx});
        //     return idx;
        // }

        // return it->second;
    }

    void CMeshBuilder::AddFace(Math::Vec3f _v1, Math::Vec3f _v2, Math::Vec3f _v3, Math::Vec3f _v4, Math::Vec3f _normal, int _color, Material _material)
    {
        auto it = m_Surfaces.find((uintptr_t)_material.get());
        if(it == m_Surfaces.end())
        {
            SIndexedSurface surface(m_SurfaceFactory());
            surface.Surface->FaceMaterial = _material;
            it = m_Surfaces.insert({(uintptr_t)_material.get(), surface}).first;
        }

        auto surface = &it->second;

        Math::Vec3f faceNormal = (_v2 - _v1).cross(_v3 - _v1).normalize();

        // 4 UVs are needed for the case, that no colorpalette is available.
        Math::Vec2f uv1, uv2, uv3, uv4;
        
        if(m_Textures && !m_Textures->empty() && !m_TextureMap)
            uv1 = uv2 = uv3 = uv4 = Math::Vec2f(((float)(_color + 0.5f)) / m_Textures->at(TextureType::DIFFIUSE)->GetSize().x, 0.5f);
        else if(m_TextureMap)
        {
            const SUVMapping *map = m_TextureMap->GetVoxelFaceInfo(_color, _normal);

            if(map)
            {
                uv1 = map->TopLeft; 
                uv2 = map->TopRight;
                uv3 = map->BottomLeft;
                uv4 = map->BottomRight;
            }
        }
        else
        {
            uv1 = Math::Vec2f(_color, 0);
            uv2 = Math::Vec2f(_color, 2);
            uv3 = Math::Vec2f(_color, 1);
            uv4 = Math::Vec2f(_color, 3);
        }

        int i1, i2, i3, i4;
        i1 = AddVertex(SVertex(_v1, _normal, uv1), *surface);
        i2 = AddVertex(SVertex(_v2, _normal, uv2), *surface);
        i3 = AddVertex(SVertex(_v3, _normal, uv3), *surface);
        i4 = AddVertex(SVertex(_v4, _normal, uv4), *surface);

        surface->Surface->ReserveFaces(surface->Surface->GetFaceCount() + 2);

        // Checks the direction of the face.
        if(faceNormal == _normal)
        {
            surface->Surface->AddFace(i1, i2, i3);
            surface->Surface->AddFace(i2, i4, i3);
        }
        else
        {
            surface->Surface->AddFace(i3, i2, i1);
            surface->Surface->AddFace(i3, i4, i2);
        }
    }
   
    void CMeshBuilder::AddFace(SVertex v1, SVertex v2, SVertex v3, Material _material)
    {        
        auto it = m_Surfaces.find((uintptr_t)_material.get());
        if(it == m_Surfaces.end())
        {
            SIndexedSurface surface(m_SurfaceFactory());
            surface.Surface->FaceMaterial = _material;
            it = m_Surfaces.insert({(uintptr_t)_material.get(), surface}).first;
        }

        int i1, i2, i3;
        i1 = AddVertex(v1, it->second);
        i2 = AddVertex(v2, it->second);
        i3 = AddVertex(v3, it->second);

        it->second.Surface->AddFace(i1, i2, i3);
    }

    Mesh CMeshBuilder::Build()
    {
        auto ret = std::make_shared<SMesh>();
        for (auto &&surface : m_Surfaces)
            ret->Surfaces.push_back(std::move(surface.second.Surface));

        ret->Textures = *m_Textures;
        m_Textures = nullptr;
        
        // Clears the cache.
        m_Surfaces.clear();

        return ret;
    }

    Mesh CMeshBuilder::Merge(Mesh _MergeInto, const std::vector<Mesh> &_Meshes, bool _ApplyModelMatrix)
    {
        Mesh ret;
        if(_MergeInto)
        {
            GenerateCache(_MergeInto);
            ret = _MergeInto;
        }
        else
        {
            ret = std::make_shared<SMesh>();
            if(!_Meshes.empty())
                ret->Textures = _Meshes[0]->Textures;
        }

        // for (auto &&m : _Meshes)
        // {
        //     for(auto &&surface : m->Surfaces)
        //     {
        //         auto it = m_Surfaces.find((size_t)surface.FaceMaterial.get());
        //         if(it == m_Surfaces.end())
        //             it = m_Surfaces.insert({(size_t)surface.FaceMaterial.get(), SIndexedSurface(surface.FaceMaterial)}).first;

        //         it->second.Surface.Vertices.reserve(it->second.Surface.Vertices.capacity() + surface.Vertices.size());
        //         it->second.Surface.Indices.reserve(it->second.Surface.Indices.capacity() + surface.Indices.size());
        //     }

        // }

        for (auto &&m : _Meshes)       
            MergeIntoThis(m, _ApplyModelMatrix);

        ret->Surfaces.clear();
        for (auto &&surface : m_Surfaces)
            ret->Surfaces.push_back(std::move(surface.second.Surface));

        // Clears the cache.
        m_Surfaces.clear();

        return ret;
    }

    bool CMeshBuilder::IsOnBorder(const Math::Vec3f &_Pos)
    {
        for (size_t i = 0; i < 3; i++)
        {
            int pos = _Pos.v[i] - ((int)(_Pos.v[i] / (float)CHUNK_SIZE) * CHUNK_SIZE);

            // TODO: Should I ever make the chunk size dynamically, than must this be also dynamic.
            if(pos == 0 || pos == (CHUNK_SIZE - 1))
                return true;
        }

        return false;
    }

    void CMeshBuilder::GenerateCache(Mesh _MergeInto)
    {
        m_Textures = &_MergeInto->Textures;

        for (auto &&surface : _MergeInto->Surfaces)
        {
            auto it = m_Surfaces.find((uintptr_t)surface->FaceMaterial.get());
            if(it == m_Surfaces.end())
                it = m_Surfaces.insert({(uintptr_t)surface->FaceMaterial.get(), SIndexedSurface(nullptr)}).first;
            
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

    uint32_t CMeshBuilder::AddMergeVertex(const SVertex &_Vertex, SIndexedSurface &_Surface, ankerl::unordered_dense::map<SVertex, int, VertexHasher> &_Index)
    {
        int idx;
        if(IsOnBorder(_Vertex.Pos))
            idx = AddVertex(_Vertex, _Surface);
        else
        {
            auto it = _Index.find(_Vertex);
            if(it != _Index.end())
                idx = it->second;
            else
            {
                idx = _Surface.Surface->GetVertexCount();
                _Surface.Surface->AddVertex(_Vertex);
                _Index.insert({_Vertex, idx});
            }
        }

        return idx;
    }

    void CMeshBuilder::MergeIntoThis(Mesh m, bool _ApplyModelMatrix)
    {
        Math::Mat4x4 rotation;
        static ankerl::unordered_dense::map<SVertex, int, VertexHasher> localIndex;

        if(_ApplyModelMatrix)
        {
            auto euler = m->ModelMatrix.GetEuler();
            rotation
                .Rotate(Math::Vec3f(0, 0, 1), euler.z)
                .Rotate(Math::Vec3f(1, 0, 0), euler.x)
                .Rotate(Math::Vec3f(0, 1, 0), euler.y);
        }

        for (auto &&surface : m->Surfaces)
        {
            auto it = m_Surfaces.find((uintptr_t)surface->FaceMaterial.get());
            if(it == m_Surfaces.end())
            {
                auto newSurface = m_SurfaceFactory();
                newSurface->FaceMaterial = surface->FaceMaterial;

                it = m_Surfaces.insert({(uintptr_t)surface->FaceMaterial.get(), SIndexedSurface(newSurface)}).first;
            }

            if(!_ApplyModelMatrix)
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

                    if(_ApplyModelMatrix)
                    {
                        v1.Pos = m->ModelMatrix * v1.Pos;
                        v1.Normal = rotation * v1.Normal;

                        v2.Pos = m->ModelMatrix * v2.Pos;
                        v2.Normal = rotation * v2.Normal;

                        v3.Pos = m->ModelMatrix * v3.Pos;
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
