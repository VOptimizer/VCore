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

#ifndef MAGICAVOXELSTREAMABLE_HPP
#define MAGICAVOXELSTREAMABLE_HPP

#include <VCore/Formats/Streamable.hpp>
#include <VCore/Misc/FileStream.hpp>
#include <VCore/Meshing/Material.hpp>
#include "MagicaVoxelModelParser.hpp"

namespace VCore
{
    class CMagicaVoxelStreamable : public IStreamable
    {
        public:
            CMagicaVoxelStreamable(
                const std::shared_ptr<IIOHandler> &p_IOHandler, 
                const std::shared_ptr<uint32_t[]> &p_Colorpalette,
                uint64_t p_ModelPosition,
                const std::shared_ptr<MaterialMap> &p_MaterialMap,
                const std::string &p_FilePath) : 
                IStreamable(),
                m_IOHandler(p_IOHandler), 
                m_Colorpalette(p_Colorpalette),
                m_ModelPosition(p_ModelPosition),
                m_MaterialMap(p_MaterialMap),
                m_FilePath(p_FilePath) {}

            /** @see IStreamable::SupportsChunkOffloading */
            bool SupportsChunkOffloading() const override { return false; }

            /** @see IStreamable::ReadVoxelSpace */
            bool ReadVoxelSpace(CVoxelSpace &p_Space) override;

        private:
            std::shared_ptr<IIOHandler> m_IOHandler;
            std::shared_ptr<uint32_t[]> m_Colorpalette;
            uint64_t m_ModelPosition;
            const std::shared_ptr<MaterialMap> m_MaterialMap;
            const std::string m_FilePath;
    };
} // namespace VCore

#endif