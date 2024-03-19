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

#include <chrono>
#include <future>

#include "Slicer/Slicer.hpp"
#include <VCore/Meshing/MeshBuilder.hpp>
#include <vector>

#include "GreedyMesher.hpp"

namespace VCore
{
    template<typename R>
    bool is_ready(std::future<R> const& f)
    { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }

    std::vector<SMeshChunk> CGreedyMesher::GenerateChunks(VoxelModel _Mesh, bool _OnlyDirty)
    {
       std::vector<SMeshChunk> ret;

        CVoxelSpace::querylist chunks;
        if(m_Frustum)
            chunks = _Mesh->QueryChunks(m_Frustum);
        else
        {
            if(!_OnlyDirty)
                chunks = _Mesh->QueryChunks();
            else
                chunks = _Mesh->QueryDirtyChunks();
        }

        CSliceCollection collection;

        std::vector<std::future<CSliceCollection>> futures;
        for (auto &&c : chunks)
        {
            _Mesh->GetVoxels().markAsProcessed(c);
            futures.push_back(std::async(&CGreedyMesher::GenerateSlicedChunk, this, _Mesh, c, true));
            while(futures.size() >= std::thread::hardware_concurrency())
            {
                auto it = futures.begin();
                while (it != futures.end())
                {
                    if(is_ready(*it))
                    {
                        auto result = it->get();
                        collection.Merge(result);                     
                        it = futures.erase(it);
                    }
                    else
                        it++;
                }
            }
        }

        auto it = futures.begin();
        while (it != futures.end())
        {
            it->wait();
            auto result = it->get();
            collection.Merge(result);       
            it = futures.erase(it);
        }

        CMeshBuilder builder;
        auto textures = _Mesh->Textures;
        auto &materials = _Mesh->Materials;

        collection.Optimize(m_GenerateTexture);
        if(m_GenerateTexture)
            textures = collection.Textures;

        builder.AddTextures(textures);

        // Generate the mesh.
        for (size_t runAxis = 0; runAxis < 3; runAxis++)
        {
            auto &slices = collection.mSlices[runAxis];
            for (auto &&slice : slices) // std::map<int, Slice>;
            {
                for (auto &&height : slice.second) // std::map<int, Quads>;
                {
                    for (auto &&quad : height.second)
                    {
                        int heightAxis = (runAxis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
                        int widthAxis = (runAxis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y

                        Math::Vec3f du;
                        du.v[widthAxis] = quad.mQuad.second.v[widthAxis];

                        Math::Vec3f dv;
                        dv.v[heightAxis] = quad.mQuad.second.v[heightAxis];

                        Math::Vec3f v1 = quad.mQuad.first;
                        Math::Vec3f v2 = quad.mQuad.first + du;
                        Math::Vec3f v3 = quad.mQuad.first + dv;
                        Math::Vec3f v4 = quad.mQuad.first + quad.mQuad.second;

                        Material mat;
                        if(quad.Material < (int)materials.size())
                            mat = materials[quad.Material];

                        if(m_GenerateTexture)
                        {
                            Math::Vec3f faceNormal = (v2 - v1).cross(v3 - v1).normalize();

                            if(faceNormal == quad.Normal)
                            {
                                builder.AddFace(
                                    SVertex(v1, quad.Normal, Math::Vec2f(quad.UvStart) / Math::Vec2f(collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    SVertex(v2, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(du.v[widthAxis], 0)) / Math::Vec2f(collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    SVertex(v3, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(0, -dv.v[heightAxis])) / Math::Vec2f(collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    mat);

                                builder.AddFace(
                                    SVertex(v2, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(du.v[widthAxis], 0)) / Math::Vec2f(collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    SVertex(v4, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(du.v[widthAxis], -dv.v[heightAxis])) / Math::Vec2f(collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    SVertex(v3, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(0, -dv.v[heightAxis])) / Math::Vec2f(collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    mat);
                            } 
                            else
                            {
                                builder.AddFace(
                                    SVertex(v3, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(0, -dv.v[heightAxis])) / Math::Vec2f(collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    SVertex(v2, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(du.v[widthAxis], 0)) / Math::Vec2f(collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    SVertex(v1, quad.Normal, Math::Vec2f(quad.UvStart) / Math::Vec2f(collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    mat);

                                builder.AddFace(
                                    SVertex(v3, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(0, -dv.v[heightAxis])) / Math::Vec2f(collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    SVertex(v4, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(du.v[widthAxis], -dv.v[heightAxis])) / Math::Vec2f(collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    SVertex(v2, quad.Normal, Math::Vec2f(quad.UvStart + Math::Vec2ui(du.v[widthAxis], 0)) / Math::Vec2f(collection.Textures[TextureType::DIFFIUSE]->GetSize())),
                                    mat);
                            }
                        }
                        else
                            builder.AddFace(v1, v2, v3, v4, quad.Normal, quad.Color, mat);
                    }
                }
            }
        }

        // We only have always one chunk using this technique.
        SMeshChunk chunk;
        Math::Vec3iHasher hasher;
        chunk.UniqueId = hasher(_Mesh->BBox.Beg);
        chunk.InnerBBox = _Mesh->BBox;
        chunk.TotalBBox = _Mesh->BBox;
        chunk.MeshData = builder.Build();

        ret.push_back(chunk);
        
        return ret;
    }

    CSliceCollection CGreedyMesher::GenerateSlicedChunk(VoxelModel m, const SChunkMeta &_Chunk, bool)
    {
        CBBox BBox = _Chunk.InnerBBox;
        CSlicer Slicer(m, _Chunk.Chunk, _Chunk.TotalBBox);
        CSliceCollection result;

        auto albedo = m->Textures[TextureType::DIFFIUSE];
        Texture emission;
        auto textureIt = m->Textures.find(TextureType::EMISSION);
        if(textureIt != m->Textures.end())
            emission = textureIt->second;

        // For all 3 axis (x, y, z)
        for (size_t Axis = 0; Axis < 3; Axis++)
        {
            Slicer.SetActiveAxis(Axis);

            int Axis1 = (Axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
            int Axis2 = (Axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y
            int x[3] = {};

            // Iterate over each slice of the 3d model.
            for (x[Axis] = BBox.Beg.v[Axis] -1; x[Axis] <= BBox.End.v[Axis];)
            {
                ++x[Axis];
                result.AddSlice(Axis, x[Axis]);

                // Foreach slice go over a 2d plane. 
                for (int HeightAxis = BBox.Beg.v[Axis1]; HeightAxis <= BBox.End.v[Axis1]; ++HeightAxis)
                {
                    for (int WidthAxis = BBox.Beg.v[Axis2]; WidthAxis <= BBox.End.v[Axis2];)
                    {
                        Math::Vec3i Pos;
                        Pos.v[Axis] = x[Axis] - 1;
                        Pos.v[Axis1] = HeightAxis;
                        Pos.v[Axis2] = WidthAxis;

                        std::map<TextureType, std::vector<CColor>> rawTextures;

                        if(Slicer.IsFace(Pos))
                        {
                            int w, h;
                            Math::Vec3i Normal = Slicer.Normal();
                            int material = Slicer.Material();
                            int Color = Slicer.Color();

                            if(m_GenerateTexture)
                            {
                                rawTextures[TextureType::DIFFIUSE].push_back(albedo->GetPixel(Math::Vec2ui(Slicer.Color(), 0)));
                                if(emission)
                                    rawTextures[TextureType::EMISSION].push_back(emission->GetPixel(Math::Vec2ui(Slicer.Color(), 0)));
                            }

                            //Claculates the width of the rect.
                            for (w = 1; WidthAxis + w <= BBox.End.v[Axis2]; w++) 
                            {
                                Math::Vec3i WPos;
                                WPos.v[Axis2] = w;

                                bool IsFace = Slicer.IsFace(Pos + WPos);
                                if(IsFace)
                                {
                                    Math::Vec3i FaceNormal = Slicer.Normal();
                                    IsFace = Normal == FaceNormal && material == Slicer.Material() && (m_GenerateTexture || Color == Slicer.Color());
                                }

                                if(!IsFace)
                                    break;
                                else if(m_GenerateTexture)
                                {
                                    rawTextures[TextureType::DIFFIUSE].push_back(albedo->GetPixel(Math::Vec2ui(Slicer.Color(), 0)));
                                    if(emission)
                                        rawTextures[TextureType::EMISSION].push_back(emission->GetPixel(Math::Vec2ui(Slicer.Color(), 0)));
                                }
                            }

                            bool done = false;
                            for (h = 1; HeightAxis + h <= BBox.End.v[Axis1]; h++)
                            {
                                // Check each block next to this quad
                                for (int k = 0; k < w; ++k)
                                {
                                    Math::Vec3i QuadPos = Pos;
                                    QuadPos.v[Axis2] += k;
                                    QuadPos.v[Axis1] += h;

                                    bool IsFace = Slicer.IsFace(QuadPos);
                                    if(IsFace)
                                    {
                                        Math::Vec3i FaceNormal = Slicer.Normal();
                                        IsFace = Normal == FaceNormal && material == Slicer.Material() && (m_GenerateTexture || Color == Slicer.Color());
                                    }

                                    // If there's a hole in the mask, exit
                                    if (!IsFace)
                                    {
                                        done = true;
                                        break;
                                    }
                                    else if(m_GenerateTexture)
                                    {
                                        rawTextures[TextureType::DIFFIUSE].push_back(albedo->GetPixel(Math::Vec2ui(Slicer.Color(), 0)));
                                        if(emission)
                                            rawTextures[TextureType::EMISSION].push_back(emission->GetPixel(Math::Vec2ui(Slicer.Color(), 0)));
                                    }
                                }

                                if (done)
                                    break;
                            }

                            x[Axis1] = HeightAxis;
                            x[Axis2] = WidthAxis;

                            int du[3] = {};
                            du[Axis2] = w;

                            int dv[3] = {};
                            dv[Axis1] = h;
                    
                            Slicer.AddProcessedQuad(Math::Vec3i(x[0], x[1], x[2]), Math::Vec3i(du[0] + dv[0], du[1] + dv[1], du[2] + dv[2]));
                            
                            if(m_GenerateTexture)
                                result.AddQuadInfo(Axis, x[Axis], x[Axis1], CQuadInfo({Math::Vec3i(x[0], x[1], x[2]), Math::Vec3i(du[0] + dv[0], du[1] + dv[1], du[2] + dv[2])}, Normal, material, rawTextures));
                            else
                                result.AddQuadInfo(Axis, x[Axis], x[Axis1], CQuadInfo({Math::Vec3i(x[0], x[1], x[2]), Math::Vec3i(du[0] + dv[0], du[1] + dv[1], du[2] + dv[2])}, Normal, material, Color));

                            // Increment counters and continue
                            WidthAxis += w;
                        }
                        else
                            WidthAxis++;
                    }
                }

                Slicer.ClearQuads();
            }

            // break;
        }

        return result;
    }
}
