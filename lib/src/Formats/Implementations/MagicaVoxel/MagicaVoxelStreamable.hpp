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

#include <VCore/Formats/Streamable.hpp>
#include <VCore/Misc/FileStream.hpp>
#include <VCore/Meshing/Material.hpp>

namespace VCore
{
    class CMagicaVoxelStreamable : public IStreamable
    {
        public:
            CMagicaVoxelStreamable(
                const std::shared_ptr<IIOHandler> &_IOHandler, 
                uint64_t _ColorpalettePosition, 
                uint64_t _ModelPosition,
                const std::shared_ptr<ankerl::unordered_dense::map<uint8_t, CMaterial>> &_NotDefaultMaterials,
                const std::string &_FilePath) :
                m_IOHandler(_IOHandler), 
                m_ColorpalettePosition(_ColorpalettePosition),
                m_ModelPosition(_ModelPosition),
                m_NotDefaultMaterials(_NotDefaultMaterials),
                m_FilePath(_FilePath) {}

            /** @see IStreamable::SupportsChunkOffloading */
            bool SupportsChunkOffloading() const override { return false; }

            /** @see IStreamable::ReadVoxelSpace */
            bool ReadVoxelSpace(CVoxelSpace &_Space) override;

        private:
            std::shared_ptr<IIOHandler> m_IOHandler;
            uint64_t m_ColorpalettePosition;
            uint64_t m_ModelPosition;
            std::shared_ptr<ankerl::unordered_dense::map<uint8_t, CMaterial>> m_NotDefaultMaterials;
            std::string m_FilePath;
    };
} // namespace VCore
