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
#include "Implementations/GreedyChunkedMesher.hpp"
#include "Implementations/GreedyMesher.hpp"
#include <VCore/Meshing/IMesher.hpp>
#include "Implementations/MarchingCubesMesher.hpp"
#include <VCore/Meshing/MeshBuilder.hpp>
#include "Implementations/SimpleMesher.hpp"
#include <future>

namespace VCore
{
    template<typename R>
    bool is_ready(std::future<R> const& f)
    { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }

    std::vector<Mesh> IMesher::GenerateScene(SceneNode sceneTree, bool mergeChilds)
    {
        return GenerateScene(sceneTree, Math::Mat4x4(), mergeChilds);
    }

    std::vector<Mesh> IMesher::GenerateAnimation(VoxelAnimation _Anim)
    {
        std::vector<Mesh> ret;

        for (size_t i = 0; i < _Anim->GetFrameCount(); i++)
        {
            auto voxelFrame = _Anim->GetFrame(i);

            auto mesh = GenerateMesh(voxelFrame.Model);
            mesh->FrameTime = voxelFrame.FrameTime;

            ret.push_back(mesh);
        }

        return ret;
    }

    std::vector<SMeshChunk> IMesher::GenerateChunks(VoxelModel _Mesh, bool _OnlyDirty)
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

        std::vector<std::future<SMeshChunk>> futures;
        for (auto &&c : chunks)
        {
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
                        result.MeshData->FrameTime = 0;
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
            result.MeshData->FrameTime = 0;
            ret.push_back(result);
            it = futures.erase(it);
        }
        
        return ret;
    }

    Mesh IMesher::GenerateMesh(VoxelModel m)
    {
        auto chunks = GenerateChunks(m);
        if(chunks.empty())
            return nullptr;
        else if(chunks.size() == 1)
        {
            auto ret = chunks[0].MeshData;
            if(ret)
            {
                ret->Name = m->Name;
                ret->FrameTime = 0;
            }

            return ret;
        }

        Mesh ret;
        size_t idx = 0;

        std::vector<Mesh> meshes(chunks.size() - 1, nullptr);
        for (auto &&c : chunks)
        {
            if(!ret)
                ret = c.MeshData;
            else
            {
                meshes[idx] = c.MeshData;
                idx++;
            }
        }

        CMeshBuilder builder;
        ret = builder.Merge(ret, meshes);
        ret->Name = m->Name;
        ret->FrameTime = 0;

        return ret;
    }

    std::vector<Mesh> IMesher::GenerateScene(SceneNode sceneTree, Math::Mat4x4 modelMatrix, bool mergeChilds)
    {
        std::vector<Mesh> ret;

        if(!mergeChilds)
            modelMatrix = modelMatrix * sceneTree->GetModelMatrix();
        else
            modelMatrix = sceneTree->GetModelMatrix();

        if(sceneTree->Mesh)
        {
            auto mesh = GenerateMesh(sceneTree->Mesh);
            if(mesh)
            {
                mesh->ModelMatrix = modelMatrix;
                ret.push_back(mesh);
            }
        }
        else if(sceneTree->Animation)
        {
            auto meshes = GenerateAnimation(sceneTree->Animation);
            for (auto &&m : meshes)
            {
                m->ModelMatrix = modelMatrix;
                ret.push_back(m);
            }
        }

        for (auto &&node : *sceneTree)
        {
            auto res = GenerateScene(node, modelMatrix, mergeChilds);

            if(!mergeChilds || !sceneTree->Mesh)
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
            case MesherTypes::GREEDY_CHUNKED: return std::make_shared<CGreedyChunkedMesher>();
            case MesherTypes::GREEDY_TEXTURED: return std::make_shared<CGreedyMesher>(true);
            default:
                throw std::runtime_error("Invalid mesher type!");
        }
    }
}
