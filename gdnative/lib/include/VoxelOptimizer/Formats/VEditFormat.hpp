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

#ifndef VEDITFORMAT_HPP
#define VEDITFORMAT_HPP

#include <map>
#include <string>
#include <string.h>
#include <tuple>
#include <VoxelOptimizer/Misc/BinaryStream.hpp>
#include <VoxelOptimizer/Formats/IVoxelFormat.hpp>
#include <VoxelOptimizer/Meshing/Material.hpp>
#include <VoxelOptimizer/Voxel/PlanesVoxelizer.hpp>

namespace VoxelOptimizer
{
    class CVEditFormat : public IVoxelFormat
    {
        public:
            CVEditFormat() = default;

            std::vector<char> Save(const std::vector<VoxelMesh> &meshes) override;

            /**
             * @brief Adds a new Texture planes section.
             * 
             * @param _name: Name of the section
             * @param _planes: Different side texture
             * @param _planesInfo: Infos of the textures like size and position inside of _planes
             */
            inline void AddTexturePlanes(const std::string &_name, Texture _planes, const SPlanesInfo &_planesInfo, const CVectori &_size)
            {
                m_TexturePlanes[_name] = std::make_tuple(_planes, _planesInfo, _size);
            }

            /**
             * @return Returns all texture planes which are associated with this file.
             */
            inline const std::map<std::string, std::tuple<Texture, SPlanesInfo, CVectori>> &GetTexturePlanes() const
            {
                return m_TexturePlanes;
            }

            ~CVEditFormat() = default;

        protected:
            std::map<std::string, std::tuple<Texture, SPlanesInfo, CVectori>> m_TexturePlanes;

            void ParseFormat() override;
    };
} // namespace VoxelOptimizer

#endif //VEDITFORMAT_HPP