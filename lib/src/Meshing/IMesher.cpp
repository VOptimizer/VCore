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
#include "Implementations/FloodMesher.hpp"

namespace VoxelOptimizer
{
    std::list<Mesh> IMesher::GenerateScene(SceneNode sceneTree, bool mergeChilds)
    {
        return GenerateScene(sceneTree, CMat4x4(), mergeChilds);
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
        builder.Merge(ret, meshes);

        return builder.Build();
    }

    std::list<Mesh> IMesher::GenerateScene(SceneNode sceneTree, CMat4x4 modelMatrix, bool mergeChilds)
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

    Mesher IMesher::Create(MesherTypes type)
    {
        switch (type)
        {
            case MesherTypes::SIMPLE: return std::make_shared<CSimpleMesher>();
            case MesherTypes::GREEDY: return std::make_shared<CGreedyMesher>();
            case MesherTypes::MARCHING_CUBES: return std::make_shared<CMarchingCubesMesher>();
            case MesherTypes::FLOOD: return std::make_shared<CFloodMesher>();
            default:
                throw std::runtime_error("Invalid mesher type!");
        }
    }
} // namespace VoxelOptimizer
