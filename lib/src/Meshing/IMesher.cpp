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

#include <memory>
#include <stdexcept>
#include "Implementations/GreedyMesher.hpp"
#include <VCore/Meshing/IMesher.hpp>
#include <VCore/Meshing/Mesh/MeshBuilder.hpp>
#include "Implementations/SimpleMesher.hpp"
#include <VCore/Formats/SceneNode.hpp>
#include <VCore/Meshing/Mesh/Mesh.hpp>
#include <future>

namespace VCore
{
    template<typename R>
    bool is_ready(std::future<R> const& f)
    { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }

    RenderSceneTree IMesher::GenerateScene(VoxelSceneTree p_SceneTree)
    {
        auto result = std::make_shared<RenderSceneTree_t>(p_SceneTree->GetChildren());

        auto models = p_SceneTree->GetModels();
        for (auto &&model : models) 
        {
            auto mesh = GenerateMesh(model);
            result->AddModel(mesh);
            model->Unload();
        }

        return result;
    }

    fast_vector<SMeshChunk> IMesher::GenerateChunks(VoxelModel p_Mesh, bool p_OnlyDirty)
    {
        fast_vector<SMeshChunk> ret;

        CVoxelSpace::querylist chunks;
        if(m_Frustum)
            chunks = p_Mesh->queryChunks(m_Frustum);
        else
        {
            if(!p_OnlyDirty)
                chunks = p_Mesh->queryChunks();
            else
                chunks = p_Mesh->queryDirtyChunks();
        }

        std::vector<std::future<SMeshChunk>> futures;
        for (auto &&c : chunks)
        {
            p_Mesh->markAsProcessed(c);
            futures.push_back(std::async(&IMesher::GenerateMeshChunk, this, p_Mesh, c, true));
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

    Mesh IMesher::GenerateMesh(VoxelModel p_Model)
    {
        auto chunks = GenerateChunks(p_Model);
        if(chunks.empty())
            return nullptr;
        else if(chunks.size() == 1)
        {
            auto ret = chunks[0].MeshData;
            if(ret)
            {
                // ret->Name = p_Model->Name;
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

        CMeshBuilder builder(m_SurfaceFactory);
        ret = builder.Merge(ret, meshes);
        // ret->Name = p_Model->Name;
        ret->FrameTime = 0;

        return ret;
    }

    void IMesher::SetFrustum(const CFrustum *p_Frustum)
    {
        if(p_Frustum && !m_Frustum)
            m_Frustum = new CFrustum(*p_Frustum);
        else if(p_Frustum)
            *m_Frustum = *p_Frustum;
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

    Mesher IMesher::Create(MesherTypes p_Type)
    {
        switch (p_Type)
        {
            case MesherTypes::SIMPLE: return std::make_shared<CSimpleMesher>();
            case MesherTypes::GREEDY: return std::make_shared<CGreedyMesher>();
            case MesherTypes::GREEDY_CHUNKED: return std::make_shared<CGreedyMesher>(false, true);
            case MesherTypes::GREEDY_TEXTURED: return std::make_shared<CGreedyMesher>(true);
            default:
                throw std::runtime_error("Invalid mesher type!");
        }
    }
}
