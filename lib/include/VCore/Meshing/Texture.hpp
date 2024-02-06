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

#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <map>
#include <memory>
#include <stddef.h>
#include <vector>
#include <VCore/Meshing/Color.hpp>
#include <VCore/Math/Vector.hpp>

namespace VCore
{
    enum class TextureType
    {
        DIFFIUSE,
        EMISSION
    };

    class CTexture
    {
        public:
            CTexture() = default;
            CTexture(const Math::Vec2ui &_Size);
            CTexture(const CTexture &_Texture);
            CTexture(const Math::Vec2ui &_Size, uint32_t *_Data);

            void AddPixel(const CColor &_Color, const Math::Vec2ui &_Position);
            void AddPixel(const CColor &_Color);

            void AddRawPixels(const std::vector<CColor> &_Pixels, const Math::Vec2ui &_Position, const Math::Vec2ui &_Size);

            inline Math::Vec2ui GetSize() const
            {
                return m_Size;
            }

            const std::vector<uint32_t> &GetPixels() const
            {
                return m_Pixels;
            }

            uint32_t GetPixel(const Math::Vec2ui &_Position);
            std::vector<char> AsPNG();

            ~CTexture() = default;
        private:
            Math::Vec2ui m_Size;
            std::vector<uint32_t> m_Pixels;
    };

    using Texture = std::shared_ptr<CTexture>;
}

#endif //TEXTURE_HPP