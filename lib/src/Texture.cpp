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

#include <VCore/Meshing/Texture.hpp>
#include <string.h>
#include <stb_image_write.h>
#include <stdexcept>

namespace VCore
{
    CTexture::CTexture(const Math::Vec2ui &_Size)
    {
        m_Size = _Size;
        m_Pixels.resize(m_Size.x * m_Size.y, 0xFF000000);
    }

    CTexture::CTexture(const CTexture &_Texture)
    {
        m_Size = _Texture.m_Size;
        m_Pixels.resize(m_Size.x * m_Size.y);

        memcpy(&m_Pixels[0], &_Texture.m_Pixels[0], m_Pixels.size() * sizeof(uint32_t));
    }

    CTexture::CTexture(const Math::Vec2ui &_Size, uint32_t *_Data)
    {
        m_Size = _Size;
        m_Pixels.resize(m_Size.x * m_Size.y);

        memcpy(&m_Pixels[0], _Data, m_Pixels.size() * sizeof(uint32_t));
    }

    void CTexture::AddPixel(const CColor &_Color, const Math::Vec2ui &_Position)
    {
        if(_Position >= m_Size)
            return;

        m_Pixels[_Position.x + m_Size.x * _Position.y] = _Color.AsRGBA();
    }

    void CTexture::AddPixel(const CColor &_Color)
    {
        if(m_Size.y != 0 && m_Size.y != 1)
            return;

        m_Size.y = 1;
        m_Size.x++;

        m_Pixels.push_back(_Color.AsRGBA());
    }

    void CTexture::AddRawPixels(const std::vector<CColor> &_Pixels, const Math::Vec2ui &_Position, const Math::Vec2ui &_Size)
    {
        if((_Position.x >= m_Size.x || _Position.y >= m_Size.y) ||
           ((_Position.x + _Size.x > m_Size.x) || _Position.y + _Size.y > m_Size.y) ||
           _Pixels.size() < (_Size.x * _Size.y))
            return;

        for (size_t y = _Position.y; y < _Position.y + _Size.y; y++)
        {
            for (size_t x = _Position.x; x < _Position.x + _Size.x; x++)
            {
                m_Pixels[x + m_Size.x * y] = _Pixels[(x - _Position.x) + _Size.x * (y - _Position.y)].AsRGBA();
            }
        }
    }

    uint32_t CTexture::GetPixel(const Math::Vec2ui &_Position)
    {
        if(_Position.x >= m_Size.x || _Position.y >= m_Size.y)
            throw std::runtime_error("Position out of bounds!");

        return m_Pixels[_Position.x + m_Size.x * _Position.y];
    }

    std::vector<char> CTexture::AsPNG()
    {
        std::vector<char> Texture;
        stbi_write_png_to_func([](void *context, void *data, int size){
            std::vector<char> *InnerTexture = (std::vector<char>*)context;
            InnerTexture->insert(InnerTexture->end(), (char*)data, ((char*)data) + size);
        }, &Texture, m_Size.x, m_Size.y, 4, m_Pixels.data(), sizeof(uint32_t) * m_Size.x);

        return Texture;
    }
}
