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

#include <fstream>
#include <stdexcept>
#include <VoxelOptimizer/Exceptions.hpp>
#include <VoxelOptimizer/Formats/IVoxelFormat.hpp>

#include "Implementations/VEditFormat.hpp"

namespace VoxelOptimizer
{
    VoxelFormat IVoxelFormat::Create(LoaderTypes type)
    {
        switch (type)
        {
            case LoaderTypes::VEDIT: return VoxelFormat(new CVEditFormat());
            // case LoaderTypes::MAGICAVOXEL: return Loader(new CMagicaVoxelLoader());
            // case LoaderTypes::GOXEL: return Loader(new CGoxelLoader());
            // case LoaderTypes::KENSHAPE: return Loader(new CKenshapeLoader());
            // case LoaderTypes::QUBICLE_BIN: return Loader(new CQubicleBinaryLoader());
            // case LoaderTypes::QUBICLE_BIN_TREE: return Loader(new CQubicleBinaryTreeLoader());
            // case LoaderTypes::QUBICLE_EXCHANGE: return Loader(new CQubicleExchangeLoader());
            // case LoaderTypes::QUBICLE: return Loader(new CQubicleLoader());

            default: throw CVoxelLoaderException("Unknown file type!");
        }
    }

    void IVoxelFormat::Save(const std::string &path, const std::vector<VoxelMesh> &meshes)
    {
        std::ofstream out(path, std::ios::binary);
        if(!out.is_open())
            throw CVoxelLoaderException("Failed to open '" + path + "'");

        try
        {
            auto File = Save(meshes);
            out.write(File.data(), File.size());
        }
        catch(const std::exception& e)
        {
            out.close();
            throw;
        }
        
        out.close();
    }

    std::vector<char> IVoxelFormat::Save(const std::vector<VoxelMesh> &meshes)
    {
        throw std::runtime_error("IVoxelFormat::Save not implemented!");
    }
} // namespace VoxelOptimizer
