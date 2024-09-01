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

#ifndef MESH_HPP
#define MESH_HPP

#include "Surface.hpp"
#include <VCore/Math/Mat4x4.hpp>
#include <VCore/Misc/fast_vector.hpp>
#include <VCore/Meshing/Texture.hpp>

namespace VCore
{
    struct SMesh
    {
        SMesh() = default;
        SMesh(const SMesh &) = delete;
        SMesh(SMesh &&) = default;
        SMesh &operator=(SMesh &&) = default;
        SMesh &operator=(const SMesh &) = delete;

        fast_vector<ISurface*> Surfaces;                                        //!< All surfaces of this mesh.
        ankerl::unordered_dense::map<TextureType, Texture> Textures;            //!< Texture used by this mesh.
        Math::Mat4x4 ModelMatrix;                                               //!< Modelmatrix according to the voxel file.

        std::string Name;       //!< Same as of the voxel model.
        unsigned int FrameTime; //!< How long this frame should be last, in ms.

        ~SMesh()
        {
            for (auto &&surface : Surfaces)
                delete surface;
        }
    };
    using Mesh = std::shared_ptr<SMesh>;
}


#endif //MESH_HPP