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
    void CMeshBuilder::AddTextures(const std::map<TextureType, Texture> &_textures)
    {
        m_Textures = &_textures;
    }

    int CMeshBuilder::AddVertex(const SVertex &_Vertex, SIndexedSurface &_Surface)
    {
        auto it = _Surface.Index.find(_Vertex);
        if(it == _Surface.Index.end())
        {
            int idx = _Surface.Vertices.size();
            _Surface.Vertices.push_back(_Vertex);
            return idx;
        }

        return it->second;
    }

    void CMeshBuilder::AddFace(Math::Vec3f _v1, Math::Vec3f _v2, Math::Vec3f _v3, Math::Vec3f _v4, Math::Vec3f _normal, int _color, Material _material)
    {
        auto it = m_Surfaces.find(_material);
        if(it == m_Surfaces.end())
            it = m_Surfaces.insert({_material, SIndexedSurface()}).first;

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
        i1 = AddVertex(SVertex(_v1, _normal, uv1), it->second);
        i2 = AddVertex(SVertex(_v2, _normal, uv2), it->second);
        i3 = AddVertex(SVertex(_v3, _normal, uv3), it->second);
        i4 = AddVertex(SVertex(_v4, _normal, uv4), it->second);

        // Checks the direction of the face.
        if(faceNormal == _normal)
        {
            it->second.Indices.push_back(i1);
            it->second.Indices.push_back(i2);
            it->second.Indices.push_back(i3);

            it->second.Indices.push_back(i2);
            it->second.Indices.push_back(i4);
            it->second.Indices.push_back(i3);
        }
        else
        {
            it->second.Indices.push_back(i3);
            it->second.Indices.push_back(i2);
            it->second.Indices.push_back(i1);

            it->second.Indices.push_back(i3);
            it->second.Indices.push_back(i4);
            it->second.Indices.push_back(i2);
        }
    }
   
    void CMeshBuilder::AddFace(SVertex v1, SVertex v2, SVertex v3, Material _material)
    {        
        auto it = m_Surfaces.find(_material);
        if(it == m_Surfaces.end())
            it = m_Surfaces.insert({_material, SIndexedSurface()}).first;

        int i1, i2, i3;
        i1 = AddVertex(v1, it->second);
        i2 = AddVertex(v2, it->second);
        i3 = AddVertex(v3, it->second);

        it->second.Indices.push_back(i1);
        it->second.Indices.push_back(i2);
        it->second.Indices.push_back(i3);
    }

    Mesh CMeshBuilder::Build()
    {
        auto ret = std::make_shared<SMesh>();
        for (auto &&surface : m_Surfaces)
        {
            ret->Surfaces.emplace_back();
            SSurface *currentSurface = &ret->Surfaces.back();
            currentSurface->FaceMaterial = surface.first;

            currentSurface->Vertices = std::move(surface.second.Vertices);
            currentSurface->Indices = std::move(surface.second.Indices);
        }

        ret->Textures = *m_Textures;
        m_Textures = nullptr;
        
        // Clears the cache.
        m_Surfaces.clear();

        return ret;
    }

    Mesh CMeshBuilder::Merge(Mesh _MergeInto, const std::vector<Mesh> &_Meshes)
    {
        Mesh ret;
        if(_MergeInto)
        {
            GenerateCache(_MergeInto);
            ret = _MergeInto;
        }
        else
            ret = std::make_shared<SMesh>();

        for (auto &&m : _Meshes)       
            MergeIntoThis(m);

        ret->Surfaces.clear();
        for (auto &&surface : m_Surfaces)
        {
            ret->Surfaces.emplace_back();
            SSurface *currentSurface = &ret->Surfaces.back();
            currentSurface->FaceMaterial = surface.first;

            currentSurface->Vertices = std::move(surface.second.Vertices);
            currentSurface->Indices = std::move(surface.second.Indices);
        }

        // Clears the cache.
        m_Surfaces.clear();

        return ret;
    }

    void CMeshBuilder::GenerateCache(Mesh _MergeInto)
    {
        for (auto &&surface : _MergeInto->Surfaces)
        {
            auto it = m_Surfaces.find(surface.FaceMaterial);
            if(it == m_Surfaces.end())
                it = m_Surfaces.insert({surface.FaceMaterial, SIndexedSurface()}).first;

            it->second.Indices = surface.Indices;
            it->second.Vertices = surface.Vertices;

            for (auto &&i : surface.Indices)
            {
                if(i < surface.Vertices.size())
                    it->second.Index.insert({surface.Vertices[i], i});
            }
        }       
    }

    void CMeshBuilder::MergeIntoThis(Mesh m)
    {
        auto euler = m->ModelMatrix.GetEuler();
        Math::Mat4x4 rotation;
        rotation
            .Rotate(Math::Vec3f(0, 0, 1), euler.z)
            .Rotate(Math::Vec3f(1, 0, 0), euler.x)
            .Rotate(Math::Vec3f(0, 1, 0), euler.y);

        for (auto &&surface : m->Surfaces)
        {
            auto it = m_Surfaces.find(surface.FaceMaterial);
            if(it == m_Surfaces.end())
                it = m_Surfaces.insert({surface.FaceMaterial, SIndexedSurface()}).first;

            for (size_t i = 0; i < surface.Indices.size(); i += 3)
            {
                SVertex v1 = surface.Vertices[surface.Indices[i]];
                SVertex v2 = surface.Vertices[surface.Indices[i + 1]];
                SVertex v3 = surface.Vertices[surface.Indices[i + 2]];

                v1.Pos = m->ModelMatrix * v1.Pos;
                v1.Normal = rotation * v1.Normal;

                v2.Pos = m->ModelMatrix * v2.Pos;
                v2.Normal = rotation * v2.Normal;

                v3.Pos = m->ModelMatrix * v3.Pos;
                v3.Normal = rotation * v3.Normal;

                int i1 = AddVertex(v1, it->second);
                int i2 = AddVertex(v2, it->second);
                int i3 = AddVertex(v3, it->second);

                it->second.Indices.push_back(i1);
                it->second.Indices.push_back(i2);
                it->second.Indices.push_back(i3);
            }
        }
    }
}
