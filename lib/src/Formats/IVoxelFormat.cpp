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

#include <stdexcept>
#include <VCore/Misc/Exceptions.hpp>
#include <VCore/Formats/IVoxelFormat.hpp>

#include "../FileUtils.hpp"

#ifndef VCORE_BUILD_NO_MAGICAVOXEL_IMPORTER
#include "Implementations/MagicaVoxel/MagicaVoxelFormat.hpp"
#endif

#ifndef VCORE_BUILD_NO_KENSHAPE_IMPORTER
#include "Implementations/KenshapeFormat.hpp"
#endif

#ifndef VCORE_BUILD_NO_GOXEL_IMPORTER
#include "Implementations/Goxel/GoxelFormat.hpp"
#endif

#ifndef VCORE_BUILD_NO_QB_IMPORTER
#include "Implementations/Qubicle/QubicleBinaryFormat.hpp"
#endif

#ifndef VCORE_BUILD_NO_QBT_IMPORTER
#include "Implementations/Qubicle/QubicleBinaryTreeFormat.hpp"
#endif

#ifndef VCORE_BUILD_NO_QEF_IMPORTER
#include "Implementations/Qubicle/QubicleExchangeFormat.hpp"
#endif

#ifndef VCORE_BUILD_NO_QBCL_IMPORTER
#include "Implementations/Qubicle/QubicleFormat.hpp"
#endif

namespace VCore
{
    VoxelFormat IVoxelFormat::Create(LoaderType _Type)
    {
        switch (_Type)
        {
            #ifndef VCORE_BUILD_NO_MAGICAVOXEL_IMPORTER
            case LoaderType::MAGICAVOXEL: return VoxelFormat(new CMagicaVoxelFormat());
            #endif

            #ifndef VCORE_BUILD_NO_GOXEL_IMPORTER
            case LoaderType::GOXEL: return VoxelFormat(new CGoxelFormat());
            #endif

            #ifndef VCORE_BUILD_NO_KENSHAPE_IMPORTER
            case LoaderType::KENSHAPE: return VoxelFormat(new CKenshapeFormat());
            #endif

            #ifndef VCORE_BUILD_NO_QB_IMPORTER
            case LoaderType::QUBICLE_BIN: return VoxelFormat(new CQubicleBinaryFormat());
            #endif

            #ifndef VCORE_BUILD_NO_QBT_IMPORTER
            case LoaderType::QUBICLE_BIN_TREE: return VoxelFormat(new CQubicleBinaryTreeFormat());
            #endif

            #ifndef VCORE_BUILD_NO_QEF_IMPORTER
            case LoaderType::QUBICLE_EXCHANGE: return VoxelFormat(new CQubicleExchangeFormat());
            #endif

            #ifndef VCORE_BUILD_NO_QBCL_IMPORTER
            case LoaderType::QUBICLE: return VoxelFormat(new CQubicleFormat());
            #endif

            default: throw CVoxelLoaderException("Unknown file type!");
        }
    }

    void IVoxelFormat::ClearCache()
    {
        m_Models.clear();
        m_Materials.clear();
        m_Textures.clear();

        m_SceneTree = std::make_shared<CSceneNode>();
    }

    LoaderType IVoxelFormat::GetType(const std::string &_Filename)
    {
        std::string ext = GetFileExt(_Filename);
        LoaderType type = LoaderType::UNKNOWN;
        
        if(ext == "vox")
            type = LoaderType::MAGICAVOXEL;
        else if(ext == "gox")
            type = LoaderType::GOXEL;
        else if(ext == "kenshape")
            type = LoaderType::KENSHAPE;
        else if(ext == "qb")
            type = LoaderType::QUBICLE_BIN;
        else if(ext == "qbt")
            type = LoaderType::QUBICLE_BIN_TREE;
        else if(ext == "qef")
            type = LoaderType::QUBICLE_EXCHANGE;
        else if(ext == "qbcl")
            type = LoaderType::QUBICLE;

        return type;
    }

    void IVoxelFormat::Load(IIOHandler *_IOHandler, const std::string _File)
    {
        DeleteFileStream();
        m_IOHandler = std::shared_ptr<IIOHandler>(_IOHandler);
        m_DataStream = m_IOHandler->Open(_File, "rb");

        ClearCache();
        ParseFormat();
    }

    void IVoxelFormat::Open(IIOHandler *_IOHandler, const std::string _File, FileMode _Mode)
    {
        if(_Mode == FileMode::CLOSED)
            throw CVoxelLoaderException("Can't open file in closed mode!");

        DeleteFileStream();
        m_IOHandler = std::shared_ptr<IIOHandler>(_IOHandler);
        m_Mode = _Mode;

        char openMode[4] = {};
        uint8_t pos = 0;
        if((static_cast<int>(_Mode) & static_cast<int>(FileMode::READ)) || (static_cast<int>(_Mode) & static_cast<int>(FileMode::STREAMED)))
            openMode[pos++] = 'r';

        if((static_cast<int>(_Mode) & static_cast<int>(FileMode::WRITE)))
            openMode[pos++] = 'w';

        openMode[pos++] = 'b';
        m_DataStream = m_IOHandler->Open(_File, openMode);
    }

    void IVoxelFormat::Load()
    {
        if((m_Mode != FileMode::READ) && (m_Mode != FileMode::STREAMED))
            throw CVoxelLoaderException("Can't read file which isn't opened in read mode!");

        ClearCache();
        ParseFormat();
    }

    void IVoxelFormat::Save()
    {
        if(m_Mode != FileMode::WRITE)
            throw CVoxelLoaderException("Can't save to file which isn't opened in write mode!");

        WriteFormat();
    }

    void IVoxelFormat::Close()
    {
        DeleteFileStream();
    }

    void IVoxelFormat::DeleteFileStream()
    {
        if(m_IOHandler)
        {
            if(m_DataStream)
                m_IOHandler->Close(m_DataStream);

            // delete m_IOHandler;
            
            m_IOHandler = nullptr;
            m_DataStream = nullptr; 
        }
    }
}
