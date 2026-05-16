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

#include "../FileUtils.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <VCore/Export/IExporter.hpp>

#ifndef VCORE_BUILD_NO_GLTF_EXPORTER
#include "Implementations/glTF/GLTFExporter.hpp"
#endif

#ifndef VCORE_BUILD_NO_OBJ_EXPORTER
#include "Implementations/obj/WavefrontObjExporter.hpp"
#endif

#ifndef VCORE_BUILD_NO_GODOT3_EXPORTER
#include "Implementations/godot/GodotSceneExporter.hpp"
#endif

#ifndef VCORE_BUILD_NO_PLY_EXPORTER
#include "Implementations/ply/PLYExporter.hpp"
#endif

#ifndef VCORE_BUILD_NO_FBX_EXPORTER
#include "Implementations/fbx/FbxExporter.hpp"
#endif

namespace VCore
{
    Exporter IExporter::Create(ExporterType p_Type)
    {
        switch (p_Type)
        {
            #ifndef VCORE_BUILD_NO_OBJ_EXPORTER
            case ExporterType::OBJ: return Exporter(new CWavefrontObjExporter());
            #endif

            #ifndef VCORE_BUILD_NO_FBX_EXPORTER
            case ExporterType::FBX: return Exporter(new CFbxExporter());
            #endif

            #ifndef VCORE_BUILD_NO_GLTF_EXPORTER
            case ExporterType::GLTF: 
            case ExporterType::GLB:
            {
                auto tmp = Exporter(new CGLTFExporter());
                tmp->Settings->Binary = p_Type == ExporterType::GLB;

                return tmp;
            } 
            #endif

            #ifndef VCORE_BUILD_NO_PLY_EXPORTER
            case ExporterType::PLY: return Exporter(new CPLYExporter());
            #endif

            #ifndef VCORE_BUILD_NO_GODOT_EXPORTER
            case ExporterType::ESCN2: return Exporter(new CGodotSceneExporter(GodotVersion::GODOT3));
            case ExporterType::ESCN3: return Exporter(new CGodotSceneExporter(GodotVersion::GODOT4));
            #endif

            default:
                throw std::runtime_error("Invalid export type!");
        }
    }

    ExporterType IExporter::GetType(const std::string &p_Filename)
    {
        std::string ext = GetFileExt(p_Filename);
        ExporterType type = ExporterType::UNKNOWN;
        
        if(ext == "obj")
            type = ExporterType::OBJ;
        else if(ext == "gltf")
            type = ExporterType::GLTF;
        else if(ext == "glb")
            type = ExporterType::GLB;
        else if(ext == "escn2")
            type = ExporterType::ESCN2;
        else if(ext == "escn3")
            type = ExporterType::ESCN3;
        else if(ext == "ply")
            type = ExporterType::PLY;
        else if(ext == "fbx")
            type = ExporterType::FBX;

        return type;
    }

    IExporter::IExporter() : ISceneTreeVisitor<RenderSceneTree_t*>(), Settings(new CExportSettings())
    { }

    void IExporter::Save(IIOHandler *p_Handler, const std::string &p_Path, Mesh p_Mesh)
    {
        Save(p_Handler, p_Path, fast_vector<Mesh>() = { p_Mesh });
    }

    void IExporter::Save(IIOHandler *p_Handler, const std::string &p_Path, const fast_vector<Mesh> &p_Meshes)
    {
        // Names all files like the output file.
        // m_ExternalFilenames = GetFilenameWithoutExt(_Path);
        // std::string PathWithoutExt = GetPathWithoutExt(_Path);
        DeleteFileStream();
        m_IOHandler = p_Handler;
        m_Path = p_Path;

        WriteHeaderData(p_Meshes);
        WriteMeshes(p_Meshes);
        WriteFooterData();

        // WriteData(p_Path, p_Meshes);
    }

    void IExporter::Save(IIOHandler *p_Handler, const std::string &p_Path, const RenderSceneTree &p_RenderTree)
    {
        DeleteFileStream();
        m_IOHandler = p_Handler;
        m_Path = p_Path;
        m_SceneTree = p_RenderTree.get();

        WriteHeaderData(p_RenderTree->GetModels());
        if(SupportsSceneTree())
        {
            WriteMeshes(p_RenderTree->GetModels());
            TraverseTree();
        }
        else
        {
            TraverseTree();
            WriteMeshes(p_RenderTree->GetModels());
        }
        WriteFooterData();

        m_SceneTree = nullptr;
    }

    void IExporter::CalculateModelDec(uint32_t p_ModelId)
    {
        for (uint64_t i = 0; i < m_NullModels.size(); i++, m_ModelIdDec++) 
        {
            if(p_ModelId < m_NullModels[i])
                return;
        }
    }

    void IExporter::TraverseTree()
    {
        ISceneTreeVisitor<RenderSceneTree_t*>::TraverseTree();
        m_NullModels.clear();
    }

    void IExporter::TraverseNode(const CSceneNodeBase *p_Node)
    {
        m_ModelIdDec = 0;
        const CSceneModelNode *modelNode = dynamic_cast<const CSceneModelNode*>(p_Node);
        if(modelNode)
        {
            auto meshes = m_SceneTree->GetModels();

            if(modelNode->ModelId >= meshes.size())
                return;

            auto mesh = meshes[modelNode->ModelId];
            if(SupportsSceneTree())
            {
                if(!mesh)
                {
                    m_NullModels.push_back(modelNode->ModelId);
                    std::sort(m_NullModels.begin(), m_NullModels.end());
                    return;
                }

                CalculateModelDec(modelNode->ModelId);
            }
            else
            {
                if(mesh)
                    mesh->ModelMatrix = modelNode->GetGlobalTransform();
            }
        }

        ISceneTreeVisitor<RenderSceneTree_t*>::TraverseNode(p_Node);
    }

    void IExporter::WriteMeshes(const fast_vector<Mesh> &p_Meshes)
    {
        for (auto &&mesh : p_Meshes)
        {
            if(mesh) [[likely]]
                WriteMeshData(mesh);
        }
    }
    
    std::string IExporter::GetMeshName(Mesh p_Mesh, const std::string & p_Default)
    {
        auto name = p_Mesh->Name.empty() ? p_Default : p_Mesh->Name;
        if(p_Mesh->FrameTime != 0)
            name += "_" + std::to_string(p_Mesh->FrameTime);

        return name;
    }

    void IExporter::SaveTexture(const Texture &p_Texture, const std::string &p_Path, const std::string &p_Suffix)
    {
        auto path = p_Path;
        if(!p_Suffix.empty())
        {
            path = GetPathWithoutExt(path);
            path += "." + p_Suffix + ".png";
        }

        auto strm = m_IOHandler->Open(path, "wb");

        auto data = p_Texture->AsPNG();
        strm->Write(data.data(), data.size());

        m_IOHandler->Close(strm);
    }

    void IExporter::DeleteFileStream()
    {
        if(m_IOHandler)
        {
            delete m_IOHandler;
            m_IOHandler = nullptr;
        }
    }
}
