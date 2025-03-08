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

#include <VCore/Misc/Exceptions.hpp>
#include <VCore/Formats/IVoxelFormat.hpp>

#include "../FileUtils.hpp"
#include <VCore/Formats/SceneNode.hpp>
#include <VCore/Voxel/Storage/VoxelSpace.hpp>

#define VCORE_BUILD_NO_KENSHAPE_IMPORTER
#define VCORE_BUILD_NO_QB_IMPORTER
#define VCORE_BUILD_NO_QBT_IMPORTER
#define VCORE_BUILD_NO_QEF_IMPORTER
#define VCORE_BUILD_NO_QBCL_IMPORTER

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
    VoxelFormat IVoxelFormat::Create(VoxelFormatType p_Type)
    {
        switch (p_Type)
        {
            #ifndef VCORE_BUILD_NO_MAGICAVOXEL_IMPORTER
            case VoxelFormatType::MAGICAVOXEL: return VoxelFormat(new CMagicaVoxelFormat());
            #endif

            #ifndef VCORE_BUILD_NO_GOXEL_IMPORTER
            case VoxelFormatType::GOXEL: return VoxelFormat(new CGoxelFormat());
            #endif

            #ifndef VCORE_BUILD_NO_KENSHAPE_IMPORTER
            case VoxelFormatType::KENSHAPE: return VoxelFormat(new CKenshapeFormat());
            #endif

            #ifndef VCORE_BUILD_NO_QB_IMPORTER
            case VoxelFormatType::QUBICLE_BIN: return VoxelFormat(new CQubicleBinaryFormat());
            #endif

            #ifndef VCORE_BUILD_NO_QBT_IMPORTER
            case VoxelFormatType::QUBICLE_BIN_TREE: return VoxelFormat(new CQubicleBinaryTreeFormat());
            #endif

            #ifndef VCORE_BUILD_NO_QEF_IMPORTER
            case VoxelFormatType::QUBICLE_EXCHANGE: return VoxelFormat(new CQubicleExchangeFormat());
            #endif

            #ifndef VCORE_BUILD_NO_QBCL_IMPORTER
            case VoxelFormatType::QUBICLE: return VoxelFormat(new CQubicleFormat());
            #endif

            default: throw CVoxelFormatException("Unknown file type!");
        }
    }

    void IVoxelFormat::ClearCache()
    {
        SceneTree = std::make_shared<VoxelSceneTree_t>();
    }

    VoxelFormatType IVoxelFormat::GetType(const std::string &p_Filename)
    {
        std::string ext = GetFileExt(p_Filename);
        VoxelFormatType type = VoxelFormatType::UNKNOWN;
        
        if(ext == "vox")
            type = VoxelFormatType::MAGICAVOXEL;
        else if(ext == "gox")
            type = VoxelFormatType::GOXEL;
        else if(ext == "kenshape")
            type = VoxelFormatType::KENSHAPE;
        else if(ext == "qb")
            type = VoxelFormatType::QUBICLE_BIN;
        else if(ext == "qbt")
            type = VoxelFormatType::QUBICLE_BIN_TREE;
        else if(ext == "qef")
            type = VoxelFormatType::QUBICLE_EXCHANGE;
        else if(ext == "qbcl")
            type = VoxelFormatType::QUBICLE;

        return type;
    }

    void IVoxelFormat::Open(IIOHandler *p_IOHandler, const std::string p_File, FileMode p_Mode)
    {
        if(p_Mode == FileMode::CLOSED)
            throw CVoxelFormatException("Can't open file in closed mode!");

        DeleteFileStream();
        m_IOHandler = std::shared_ptr<IIOHandler>(p_IOHandler);
        m_Mode = p_Mode;

        char openMode[4] = {};
        uint8_t pos = 0;
        if((static_cast<int>(p_Mode) & static_cast<int>(FileMode::READ)) || (static_cast<int>(p_Mode) & static_cast<int>(FileMode::STREAMED)))
            openMode[pos++] = 'r';

        if((static_cast<int>(p_Mode) & static_cast<int>(FileMode::WRITE)))
            openMode[pos++] = 'w';

        openMode[pos++] = 'b';
        m_DataStream = m_IOHandler->Open(p_File, openMode);
    }

    void IVoxelFormat::Load()
    {
        if((m_Mode != FileMode::READ) && (m_Mode != FileMode::STREAMED))
            throw CVoxelFormatException("Can't read file which isn't opened in read mode!");

        ClearCache();
        ParseFormat();
    }

    void IVoxelFormat::Save()
    {
        if(m_Mode != FileMode::WRITE)
            throw CVoxelFormatException("Can't save to file which isn't opened in write mode!");

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
