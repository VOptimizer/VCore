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
#include <vector>

namespace VCore
{
    struct SRect
    {
        public:
            SRect() = default;
            SRect(const Math::Vec2ui &_Position, const Math::Vec2ui &_Size, void *_Reference) : Position(_Position), Size(_Size), Reference(_Reference) {}
            SRect(SRect &&_Other) { *this = std::move(_Other); }
            SRect(const SRect &_Other) { *this = _Other; }

            SRect &operator=(SRect &&_Other)
            {
                Position = std::move(_Other.Position);
                Size = std::move(_Other.Size);
                Reference = std::move(_Other.Reference);
                return *this;
            }

            SRect &operator=(const SRect &_Other)
            {
                Position = _Other.Position;
                Size = _Other.Size;
                Reference = _Other.Reference;
                return *this;
            }

            Math::Vec2ui Position;
            Math::Vec2ui Size;

            void *Reference; //!< Should be a Template
    };
    
    struct SNode
    {
        SNode() : Child(), Leaf(true) {}
        SNode(const Math::Vec2ui &_Position, const Math::Vec2ui &_Size) : SNode()
        {
            Position = _Position;
            Size = _Size;
        }

        SNode *Child[2];
        Math::Vec2ui Position;
        Math::Vec2ui Size;
        bool Leaf;

        ~SNode()
        {
            for (size_t i = 0; i < 2; i++)
            {
                if(Child[i])
                    delete Child[i];
            }
        }
    };
    
    class CTexturePacker
    {
        public:
            CTexturePacker() = default;

            /**
             * @brief Adds a new quad to the rect list.
             */
            void AddRect(const Math::Vec2ui &_Size, void *_Ref = nullptr);

            /**
             * @brief Packs the rects to fit into one texture.
             */
            const std::vector<SRect> &Pack();

            /**
             * @return Gets the calculated canvas size.
             */
            inline Math::Vec2ui GetCanvasSize() const
            {
                return m_CanvasSize;
            }

            ~CTexturePacker() {}
        private:
            Math::Vec2ui m_CanvasSize;
            std::vector<SRect> m_Rects;

            SNode *FindNode(SNode *_Root, const Math::Vec2ui &_Size);
            Math::Vec2ui SplitNode(SNode *_Root, const Math::Vec2ui &_Size);
            SNode *ResizeCanvas(SNode *_Root, const Math::Vec2ui &_Size);

            SNode *ResizeCanvasRight(SNode *_Root, const Math::Vec2ui &_Size);
            SNode *ResizeCanvasDown(SNode *_Root, const Math::Vec2ui &_Size);
    };    
} // namespace VCore

#endif