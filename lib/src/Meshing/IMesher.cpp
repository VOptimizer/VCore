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

#include <stdexcept>
#include "Implementations/GreedyMesher.hpp"
#include <VoxelOptimizer/Meshing/IMesher.hpp>
#include "Implementations/MarchingCubesMesher.hpp"
#include <VoxelOptimizer/Meshing/MeshBuilder.hpp>
#include "Implementations/SimpleMesher.hpp"
#include <future>

namespace VoxelOptimizer
{
    template<typename R>
    bool is_ready(std::future<R> const& f)
    { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }

    std::list<Mesh> IMesher::GenerateScene(SceneNode sceneTree, bool mergeChilds)
    {
        return GenerateScene(sceneTree, Math::Mat4x4(), mergeChilds);
    }

    std::list<SMeshChunk> IMesher::GenerateChunks(VoxelMesh _Mesh, bool _OnlyDirty, int _ChunkCount)
    {
        std::list<SMeshChunk> ret;

        // m_Voxels = _Mesh->QueryVisible(true);

        std::list<SChunkMeta> chunks;
        if(!_OnlyDirty)
            chunks = _Mesh->QueryChunks(m_Frustum);
        else
            chunks = _Mesh->QueryDirtyChunks(m_Frustum);

        std::list<std::future<SMeshChunk>> futures;
        int counter = 0;
        for (auto &&c : chunks)
        {
            if(_ChunkCount != -1)
            {
                if(counter == _ChunkCount)
                    break;

                counter++;
            }
            _Mesh->GetVoxels().markAsProcessed(c);
            futures.push_back(std::async(&IMesher::GenerateMeshChunk, this, _Mesh, c, true));
            while(futures.size() >= std::thread::hardware_concurrency())
            {
                auto it = futures.begin();
                while (it != futures.end())
                {
                    if(is_ready(*it))
                    {
                        auto result = it->get();
                        ret.push_back(result);                        
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
            ret.push_back(result);
            it = futures.erase(it);
        }

        // m_Voxels = _Mesh->QueryVisible(false);
        counter = 0;
        // if(!m_Voxels.empty())
        // {
            // for (auto &&c : chunks)        
            // {
            //     if(_ChunkCount != -1)
            //     {
            //         if(counter == _ChunkCount)
            //             break;

            //         counter++;
            //     }

            //     auto mesh = GenerateMeshChunk(_Mesh, c, false);
            //     auto it = std::find_if(ret.begin(), ret.end(), [&c](const SMeshChunk &_Chunk) {
            //         return _Chunk.UniqueId == c.UniqueId;
            //     });

            //     // Merge the transparent chunk with the opaque one.
            //     if(it != ret.end())
            //     {
            //         CMeshBuilder builder;
            //         builder.Merge(it->Mesh, std::vector<Mesh>() = { mesh.Mesh });
            //     }
            //     else
            //         ret.push_back(mesh);
            // }
        // }
        // m_Voxels.clear();
        
        return ret;
    }

    Mesh IMesher::GenerateMesh(VoxelMesh m)
    {
        auto chunks = GenerateChunks(m);
        if(chunks.empty())
            return nullptr;

        Mesh ret;
        size_t idx = 0;

        std::vector<Mesh> meshes(chunks.size() - 1, nullptr);
        for (auto &&c : chunks)
        {
            if(!ret)
                ret = c.Mesh;
            else
            {
                meshes[idx] = c.Mesh;
                idx++;
            }
        }

        CMeshBuilder builder;
        ret = builder.Merge(ret, meshes);

        return ret;
    }

    std::list<Mesh> IMesher::GenerateScene(SceneNode sceneTree, Math::Mat4x4 modelMatrix, bool mergeChilds)
    {
        std::list<Mesh> ret;

        if(!mergeChilds)
            modelMatrix = modelMatrix * sceneTree->ModelMatrix();
        else
            modelMatrix = sceneTree->ModelMatrix();

        if(sceneTree->GetMesh() && sceneTree->GetMesh()->GetBlockCount() != 0)
        {
            auto mesh = GenerateMesh(sceneTree->GetMesh());
            mesh->ModelMatrix = modelMatrix;
            ret.push_back(mesh);
        }

        for (auto &&node : *sceneTree)
        {
            auto res = GenerateScene(node, modelMatrix, mergeChilds);

            if(!mergeChilds || !sceneTree->GetMesh())
                ret.insert(ret.end(), res.begin(), res.end());
            else
            {
                CMeshBuilder builder;

                std::vector<Mesh> meshes;
                for (auto &&m : res)
                    meshes.push_back(m);

                builder.Merge(ret.back(), meshes);
                ret.back() = builder.Build();
            }
        }

        return ret;
    }

    void IMesher::SetFrustum(const CFrustum *_Frustum)
    {
        if(_Frustum && !m_Frustum)
            m_Frustum = new CFrustum(*_Frustum);
        else if(_Frustum)
            *m_Frustum = *_Frustum;
        else
        {
            if(m_Frustum)
                delete m_Frustum;
            m_Frustum = nullptr;
        }
    }

    IMesher::~IMesher()
    {
        if(m_Frustum)
            delete m_Frustum;
    }

    Mesher IMesher::Create(MesherTypes type)
    {
        switch (type)
        {
            case MesherTypes::SIMPLE: return std::make_shared<CSimpleMesher>();
            case MesherTypes::GREEDY: return std::make_shared<CGreedyMesher>();
            case MesherTypes::MARCHING_CUBES: return std::make_shared<CMarchingCubesMesher>();
            default:
                throw std::runtime_error("Invalid mesher type!");
        }
    }
}
