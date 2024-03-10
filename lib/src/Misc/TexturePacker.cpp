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
#include <cstdlib>

// Modified version of https://codeincomplete.com/articles/bin-packing/
// Which is also a modified version of https://blackpawn.com/texts/lightmaps/default.html
namespace VCore
{
    void CTexturePacker::AddRect(const Math::Vec2ui &_Size, void *_Ref)
    {
        m_Rects.emplace_back(Math::Vec2ui(), _Size, _Ref);
    }

    const std::vector<SRect> &CTexturePacker::Pack()
    {
        if(m_Rects.empty())
            return m_Rects;

        // Sorts the rects by size.
        // Begins at the smalles one and goes bigger.
        std::qsort
        (
            m_Rects.data(),
            m_Rects.size(),
            sizeof(decltype(m_Rects)::value_type),
            [](const void* x, const void* y)
            {
                const SRect arg1 = *static_cast<const SRect*>(x);
                const SRect arg2 = *static_cast<const SRect*>(y);
                if (arg1.Size.length() < arg2.Size.length())
                    return -1;
                if (arg1.Size.length() > arg2.Size.length())
                    return 1;
                return 0;
            }
        );

        m_CanvasSize = m_Rects.back().Size;
        SNode *root = new SNode(Math::Vec2ui(), m_CanvasSize);

        auto it = m_Rects.rbegin();
        while (it != m_Rects.rend())
        {
            auto node = FindNode(root, it->Size);
            if(node)
            {
                it->Position = SplitNode(node, it->Size);
                it++;
            }
            else
            {
                node = ResizeCanvas(root, it->Size);
                if(node)
                    root = node;
                else
                    break; // To prevent an endless loop.
            }
        }

        delete root;
        return m_Rects;
    }

    SNode *CTexturePacker::FindNode(SNode *_Root, const Math::Vec2ui &_Size)
    {
        if(_Root)
        {
            if(!_Root->Leaf)
            {
                auto node = FindNode(_Root->Child[0], _Size); 
                if(!node)
                    node = FindNode(_Root->Child[1], _Size);

                return node;
            }
            else if((_Size.x <= _Root->Size.x) && (_Size.y <= _Root->Size.y))
                return _Root;
        }

        return nullptr;
    }

    Math::Vec2ui CTexturePacker::SplitNode(SNode *_Root, const Math::Vec2ui &_Size)
    {
        _Root->Leaf = false;
        auto size = _Root->Size - Math::Vec2ui(0, _Size.y);
        if(size.y > 0)
            _Root->Child[0] = new SNode(_Root->Position + Math::Vec2ui(0, _Size.y), _Root->Size - Math::Vec2ui(0, _Size.y));

        size = _Root->Size - Math::Vec2ui(_Size.x, size.y);
        if(size.x > 0)
            _Root->Child[1] = new SNode(_Root->Position + Math::Vec2ui(_Size.x, 0), size);

        return _Root->Position;
    }

    SNode *CTexturePacker::ResizeCanvas(SNode *_Root, const Math::Vec2ui &_Size)
    {
        bool canGrowDown  = (_Size.x <= _Root->Size.x);
        bool canGrowRight = (_Size.y <= _Root->Size.y);

        // Checks to keep a squarish texture.
        bool shouldGrowRight = (canGrowRight && (_Root->Size.y >= (_Root->Size.x + _Size.x)));
        bool shouldGrowDown =  (canGrowRight && (_Root->Size.x >= (_Root->Size.y + _Size.y)));

        SNode *newRoot = nullptr;

        if(shouldGrowRight)
            newRoot = ResizeCanvasRight(_Root, _Size);
        else if(shouldGrowDown)
            newRoot = ResizeCanvasDown(_Root, _Size);
        else if(canGrowRight)
            newRoot = ResizeCanvasRight(_Root, _Size);
        else if(canGrowDown)
            newRoot = ResizeCanvasDown(_Root, _Size);

        return newRoot;
    }

    SNode *CTexturePacker::ResizeCanvasRight(SNode *_Root, const Math::Vec2ui &_Size)
    {
        m_CanvasSize.x += _Size.x;
        auto newRoot = new SNode(Math::Vec2ui(), m_CanvasSize);
        newRoot->Child[0] = _Root;
        newRoot->Child[1] = new SNode(Math::Vec2ui(_Root->Size.x, 0), Math::Vec2ui(_Size.x, m_CanvasSize.y));
        newRoot->Leaf = false;

        return newRoot;
    }

    SNode *CTexturePacker::ResizeCanvasDown(SNode *_Root, const Math::Vec2ui &_Size)
    {
        m_CanvasSize.y += _Size.y;

        auto newRoot = new SNode(Math::Vec2ui(), m_CanvasSize);
        newRoot->Child[1] = _Root;
        newRoot->Child[0] = new SNode(Math::Vec2ui(0, _Root->Size.y), Math::Vec2ui(m_CanvasSize.x, _Size.y));
        newRoot->Leaf = false;

        return newRoot;
    }
} // namespace VCore
