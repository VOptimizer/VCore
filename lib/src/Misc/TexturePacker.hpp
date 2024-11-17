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

#ifndef TEXTUREPACKER_HPP
#define TEXTUREPACKER_HPP

#include <VCore/Math/Vector.hpp>
#include <VCore/Memory/MemoryPool.hpp>
#include <vector>

namespace VCore
{
    template<class T>
    struct TRect
    {
        public:
            TRect() = default;
            TRect(const Math::Vec2ui &_Position, const Math::Vec2ui &_Size, T &&_Reference) : Position(_Position), Size(_Size), Reference(std::move(_Reference)) {}
            TRect(TRect &&_Other) { *this = std::move(_Other); }
            TRect(const TRect &_Other) { *this = _Other; }

            TRect &operator=(TRect &&_Other)
            {
                Position = std::move(_Other.Position);
                Size = std::move(_Other.Size);
                Reference = std::move(_Other.Reference);
                return *this;
            }

            TRect &operator=(const TRect &_Other)
            {
                Position = _Other.Position;
                Size = _Other.Size;
                Reference = _Other.Reference;
                return *this;
            }

            Math::Vec2ui Position;
            Math::Vec2ui Size;

            T Reference;
    };
    
    // Modified version of https://codeincomplete.com/articles/bin-packing/
    // Which is also a modified version of https://blackpawn.com/texts/lightmaps/default.html
    template<class T>
    class TTexturePacker
    {
        public:
            TTexturePacker() = default;

            /**
             * @brief Adds a new quad to the rect list.
             */
            inline void AddRect(const Math::Vec2ui &_Size, T &&_Ref)
            {
                m_Rects.emplace_back(Math::Vec2ui(), _Size, std::move(_Ref));
            }

            /**
             * @brief Packs the rects to fit into one texture.
             */
            inline const std::vector<TRect<T>> &Pack()
            {
                if(m_Rects.empty())
                    return m_Rects;

                // Sorts the rects by size.
                // Begins at the smalles one and goes bigger.
                std::qsort
                (
                    m_Rects.data(),
                    m_Rects.size(),
                    sizeof(TRect<T>),
                    [](const void* x, const void* y)
                    {
                        const auto arg1 = *static_cast<const TRect<T>*>(x);
                        const auto arg2 = *static_cast<const TRect<T>*>(y);
                        if (arg1.Size.length() < arg2.Size.length())
                            return -1;
                        if (arg1.Size.length() > arg2.Size.length())
                            return 1;
                        return 0;
                    }
                );

                m_CanvasSize = m_Rects.back().Size;
                // SNode *root = new SNode(Math::Vec2ui(), m_CanvasSize);
                SNode *root = m_Pool.construct(Math::Vec2ui(), m_CanvasSize, &m_Pool);

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

                // delete root;
                m_Pool.destruct(root);
                return m_Rects;
            }

            /**
             * @return Gets the calculated canvas size.
             */
            inline Math::Vec2ui GetCanvasSize() const
            {
                return m_CanvasSize;
            }

            ~TTexturePacker() {}
        private:
            struct SNode;
            CMemoryPool<SNode> m_Pool;

            struct SNode
            {
                SNode() : Child(), Pool(nullptr), Leaf(true) {}
                SNode(const Math::Vec2ui &_Position, const Math::Vec2ui &_Size, CMemoryPool<SNode> *_Pool) : SNode()
                {
                    Position = _Position;
                    Size = _Size;
                    Pool = _Pool;
                }

                SNode *Child[2];
                Math::Vec2ui Position;
                Math::Vec2ui Size;
                CMemoryPool<SNode> *Pool;
                bool Leaf;

                ~SNode()
                {
                    for (size_t i = 0; i < 2; i++)
                    {
                        if(Child[i])
                            Pool->destruct(Child[i]);
                            // delete Child[i];
                    }
                }
            };

            Math::Vec2ui m_CanvasSize;
            std::vector<TRect<T>> m_Rects;

            inline SNode *FindNode(SNode *_Root, const Math::Vec2ui &_Size)
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

            inline Math::Vec2ui SplitNode(SNode *_Root, const Math::Vec2ui &_Size)
            {
                _Root->Leaf = false;
                auto size = _Root->Size - Math::Vec2ui(0, _Size.y);
                if(size.y > 0)
                    _Root->Child[0] = m_Pool.construct(_Root->Position + Math::Vec2ui(0, _Size.y), _Root->Size - Math::Vec2ui(0, _Size.y), &m_Pool);
                    // new SNode(_Root->Position + Math::Vec2ui(0, _Size.y), _Root->Size - Math::Vec2ui(0, _Size.y));

                size = _Root->Size - Math::Vec2ui(_Size.x, size.y);
                if(size.x > 0)
                    _Root->Child[1] = m_Pool.construct(_Root->Position + Math::Vec2ui(_Size.x, 0), size, &m_Pool);
                    // new SNode(_Root->Position + Math::Vec2ui(_Size.x, 0), size);

                return _Root->Position;
            }

            inline SNode *ResizeCanvas(SNode *_Root, const Math::Vec2ui &_Size)
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

            inline SNode *ResizeCanvasRight(SNode *_Root, const Math::Vec2ui &_Size)
            {
                m_CanvasSize.x += _Size.x;
                auto newRoot = m_Pool.construct(Math::Vec2ui(), m_CanvasSize, &m_Pool);
                // new SNode(Math::Vec2ui(), m_CanvasSize);
                newRoot->Child[0] = _Root;
                newRoot->Child[1] = m_Pool.construct(Math::Vec2ui(_Root->Size.x, 0), Math::Vec2ui(_Size.x, m_CanvasSize.y), &m_Pool);
                // new SNode(Math::Vec2ui(_Root->Size.x, 0), Math::Vec2ui(_Size.x, m_CanvasSize.y));
                newRoot->Leaf = false;

                return newRoot;
            }

            inline SNode *ResizeCanvasDown(SNode *_Root, const Math::Vec2ui &_Size)
            {
                m_CanvasSize.y += _Size.y;

                auto newRoot = m_Pool.construct(Math::Vec2ui(), m_CanvasSize, &m_Pool);
                // new SNode(Math::Vec2ui(), m_CanvasSize);
                newRoot->Child[1] = _Root;
                newRoot->Child[0] = m_Pool.construct(Math::Vec2ui(0, _Root->Size.y), Math::Vec2ui(m_CanvasSize.x, _Size.y), &m_Pool);
                // new SNode(Math::Vec2ui(0, _Root->Size.y), Math::Vec2ui(m_CanvasSize.x, _Size.y));
                newRoot->Leaf = false;

                return newRoot;
            }
    };    
} // namespace VCore

#endif