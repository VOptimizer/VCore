/*
 * MIT License
 *
 * Copyright (c) 2025 Christian Tost
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

#ifndef MAGICAVOXELMODELPARSER_HPP
#define MAGICAVOXELMODELPARSER_HPP

#include <VCore/Misc/FileStream.hpp>
#include <VCore/Voxel/Storage/VoxelSpace.hpp>
#include <VCore/Meshing/Texture.hpp>
#include <VCore/Meshing/Material.hpp>
#include <cstdint>
#include <utility>

namespace VCore
{
    extern const unsigned int DefaultPalette[256];

    [[nodiscard]]
    constexpr uint32_t MakeChunkId(char p_c1, char p_c2, char p_c3, char p_c4)
    {
        return (static_cast<uint32_t>(p_c1)) |
               (static_cast<uint32_t>(p_c2) << 8) |
               (static_cast<uint32_t>(p_c3) << 16) |
               (static_cast<uint32_t>(p_c4) << 24);
    }

    struct MaterialMap
    {
        std::pair<CMaterial, uint8_t> Materials[257];
    };

    class CMagicaVoxelModelParser
    {
        public:
            CMagicaVoxelModelParser(
                IFileStream *p_Stream, 
                const std::shared_ptr<uint32_t[]> &p_Colorpalette,
                uint64_t p_ModelPosition,
                const std::shared_ptr<MaterialMap> &p_MaterialMap) 
            : m_Stream(p_Stream), m_Colorpalette(p_Colorpalette), 
            m_ModelPosition(p_ModelPosition), m_MaterialMap(p_MaterialMap) {}

            /**
             * @brief Fills a given voxel space with voxel data
             */
            void FillVoxelSpace(CVoxelSpace &p_Space);

            Math::Vec3i GetSize() const { return m_Size; }

            static Math::Vec3i ProcessSize(IFileStream *p_Stream);

            ~CMagicaVoxelModelParser() = default;
        private:
            uint32_t GetColor(uint8_t p_ColorIdx);
            uint8_t GetMaterial(uint8_t p_MaterialIdx);

            IFileStream *m_Stream;
            std::shared_ptr<uint32_t[]> m_Colorpalette;
            uint64_t m_ModelPosition;
            std::shared_ptr<MaterialMap> m_MaterialMap;
            Math::Vec3i m_Size;
    };
} // namespace VCore

#endif