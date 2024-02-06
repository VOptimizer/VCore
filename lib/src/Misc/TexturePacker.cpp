/*
 * MIT License
 *
 * Copyright (c) 2024 Christian Tost
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

#include "TexturePacker.hpp"

namespace VCore
{
    void CTexturePacker::AddRect(const Math::Vec2ui &_Size, void *_Ref)
    {
        if(m_FreeRects.empty())
        {
            SRect rect(Math::Vec2ui(), _Size, _Ref);

            SRect right(Math::Vec2ui(rect.Size.x, 0), Math::Vec2ui(m_CanvasSize.x - rect.Size.x, rect.Size.y), nullptr);
            SRect down(Math::Vec2ui(0, rect.Size.y), Math::Vec2ui(m_CanvasSize.x,  m_CanvasSize.y - rect.Size.y), nullptr);

            m_FreeRects.push_back(right);
            auto it = FindClosest(down.Size);
            m_FreeRects.insert(it, down);

            m_CurrentSize = _Size;
            m_Rects.push_back(rect);
        }
        else
        {
            auto it = FindClosest(_Size);
            if(it == m_FreeRects.end())
            {
                // TODO: Grow
            }
            else
            {
                SRect closest = *it;
                m_FreeRects.erase(it);

                SRect rect(closest.Position, _Size, _Ref);
                if(_Size.x < closest.Size.x)
                {
                    SRect left(closest.Position + Math::Vec2ui(rect.Size.x, 0), closest.Size - Math::Vec2ui(rect.Size.x, 0), nullptr);
                    auto it = FindClosest(left.Size);
                    m_FreeRects.insert(it, left);
                }

                if(_Size.y < closest.Size.y)
                {
                    SRect down(closest.Position + Math::Vec2ui(0, rect.Size.y), Math::Vec2ui(rect.Size.x, closest.Size.y - rect.Size.y), nullptr);
                    auto it = FindClosest(down.Size);
                    m_FreeRects.insert(it, down);
                }

                m_Rects.push_back(rect);
            }
        }
    }

    std::vector<SRect>::const_iterator CTexturePacker::FindClosest(const Math::Vec2ui &_Size)
    {
        if(m_FreeRects.empty())
            return m_FreeRects.end();
        
        int left = 0, right = m_FreeRects.size() - 1;
        while (left <= right)
        {
            int center = left + (right - left) / 2;
            if(m_FreeRects[center].Size == _Size)
                return m_FreeRects.begin() + center;

            if(m_FreeRects[center].Size > _Size)
                right = center - 1;
            else
                left = center + 1;
        }
        
        return m_FreeRects.begin() + left;
    }

    // std::vector<SRect>::const_iterator CTexturePacker::FindQuad(const Math::Vec2ui &_Size)
    // {
    //     int left = 0, right = m_FreeRects.size() - 1;
    //     while (left <= right)
    //     {
    //         int center = left + (right - left) / 2;
    //         if(m_FreeRects[center].Size == _Size)
    //             return m_FreeRects.begin() + center;

    //         if(m_FreeRects[center].Size > _Size)
    //             right = center - 1;
    //         else
    //             left = center + 1;
    //     }
        
    //     return m_FreeRects.end();
    // }
} // namespace VCore
