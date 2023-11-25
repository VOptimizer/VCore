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
#include <stdexcept>
#include "Implementations/glTF/GLTFExporter.hpp"
#include "Implementations/GodotSceneExporter.hpp"
#include <VCore/Export/IExporter.hpp>
#include "Implementations/WavefrontObjExporter.hpp"
#include "Implementations/PLYExporter.hpp"

namespace VCore
{
    Exporter IExporter::Create(ExporterType _Type)
    {
        switch (_Type)
        {
            case ExporterType::OBJ: return Exporter(new CWavefrontObjExporter());

            case ExporterType::GLTF: 
            case ExporterType::GLB:
            {
                auto tmp = Exporter(new CGLTFExporter());
                tmp->Settings()->Binary = _Type == ExporterType::GLB;

                return tmp;
            } 

            case ExporterType::PLY: return Exporter(new CPLYExporter());
            case ExporterType::ESCN: return Exporter(new CGodotSceneExporter());

            default:
                throw std::runtime_error("Invalid export type!");
        }
    }

    ExporterType IExporter::GetType(const std::string &_Filename)
    {
        std::string ext = GetFileExt(_Filename);
        ExporterType type = ExporterType::UNKNOWN;
        
        if(ext == "obj")
            type = ExporterType::OBJ;
        else if(ext == "gltf")
            type = ExporterType::GLTF;
        else if(ext == "glb")
            type = ExporterType::GLB;
        else if(ext == "escn")
            type = ExporterType::ESCN;
        else if(ext == "ply")
            type = ExporterType::PLY;

        return type;
    }

    IExporter::IExporter() : m_Settings(new CExportSettings())
    {

    }

    void IExporter::Save(IIOHandler *_Handler, const std::string &_Path, Mesh _Mesh)
    {
        Save(_Handler, _Path, std::vector<Mesh>() = { _Mesh });
    }

    void IExporter::Save(IIOHandler *_Handler, const std::string &_Path, const std::vector<Mesh> &_Meshes)
    {
        // Names all files like the output file.
        // m_ExternalFilenames = GetFilenameWithoutExt(_Path);
        // std::string PathWithoutExt = GetPathWithoutExt(_Path);
        DeleteFileStream();
        m_IOHandler = _Handler;
        WriteData(_Path, _Meshes);
    }

    void IExporter::SaveTexture(const Texture &_Texture, const std::string &_Path, const std::string &_Suffix)
    {
        auto path = _Path;
        if(!_Suffix.empty())
        {
            path = GetPathWithoutExt(path);
            path += "." + _Suffix + ".png";
        }

        auto strm = m_IOHandler->Open(path, "wb");

        auto data = _Texture->AsPNG();
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
