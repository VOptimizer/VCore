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

#include <VCore/Misc/FileStream.hpp>
#include <VCore/Voxel/Storage/VoxelSpace.hpp>
#include <VCore/Meshing/Texture.hpp>
#include <VCore/Meshing/Material.hpp>

namespace VCore
{
    extern const unsigned int DefaultPalette[256];

    class CMagicaVoxelModelParser
    {
        public:
            CMagicaVoxelModelParser(
                IFileStream *_Stream, 
                uint64_t _ColorpalettePosition, 
                uint64_t _ModelPosition,
                const std::shared_ptr<ankerl::unordered_dense::map<uint8_t, CMaterial>> &_NotDefaultMaterials) 
            : m_Stream(_Stream), m_ColorpalettePosition(_ColorpalettePosition), 
            m_ModelPosition(_ModelPosition), m_NotDefaultMaterials(_NotDefaultMaterials) {}

            /**
             * @brief Fills a given voxel space with voxel data
             */
            void FillVoxelSpace(CVoxelSpace &_Space);

            Math::Vec3i GetSize() const { return m_Size; }

            static Math::Vec3i ProcessSize(IFileStream *_Stream);

            ~CMagicaVoxelModelParser() = default;
        private:
            uint32_t GetColor(uint8_t _ColorIdx);
            uint8_t GetMaterial(uint8_t _MaterialIdx);

            IFileStream *m_Stream;
            uint64_t m_ColorpalettePosition;
            uint64_t m_ModelPosition;
            std::shared_ptr<ankerl::unordered_dense::map<uint8_t, CMaterial>> m_NotDefaultMaterials;
            Math::Vec3i m_Size;
    };
} // namespace VCore
