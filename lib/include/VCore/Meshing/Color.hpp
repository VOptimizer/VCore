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

#ifndef COLOR_HPP
#define COLOR_HPP

#include <cstdint>
#include <utility>

namespace VCore
{
    class CColor
    {
        public:
            union
            {
                struct
                {
                    unsigned char R;
                    unsigned char G;
                    unsigned char B;
                    unsigned char A;
                };

                unsigned char c[4];
            };

            CColor() : R(255), G(255), B(255), A(255) {}
            CColor(unsigned char p_Red, unsigned char p_Green, unsigned char p_Blue, unsigned char p_Alpha) : R(p_Red), G(p_Green), B(p_Blue), A(p_Alpha) {}
            CColor(const CColor &p_Other) { *this = p_Other; }
            CColor(CColor &&p_Other) { *this = std::move(p_Other); }

            CColor &operator=(const CColor &p_Other)
            {
                R = p_Other.R;
                G = p_Other.G;
                B = p_Other.B;
                A = p_Other.A;
                return *this;
            }

            CColor &operator=(CColor &&p_Other)
            {
                R = std::move(p_Other.R);
                G = std::move(p_Other.G);
                B = std::move(p_Other.B);
                A = std::move(p_Other.A);

                p_Other.R = 255;
                p_Other.G = 255;
                p_Other.B = 255;
                p_Other.A = 255;

                return *this;
            }

            inline void FromRGBA(uint32_t p_color)
            {
                R = p_color & 0xFF;
                G = (p_color & 0xFF00) >> 8;
                B = (p_color & 0xFF0000) >> 16;
                A = (p_color & 0xFF000000) >> 24;
            }

            inline static CColor CreateFromRGBA(uint32_t p_color)
            {
                CColor c;
                c.FromRGBA(p_color);
                return c;
            }

            inline uint32_t AsRGBA() const
            {
                return (uint32_t)R | (uint32_t)(G << 8) | (uint32_t)(B << 16) | (uint32_t)(A << 24);
            }

            inline bool operator!=(const CColor &p_Rhs)
            {
                return R != p_Rhs.R || G != p_Rhs.G || B != p_Rhs.B || A != p_Rhs.A;
            }

            ~CColor() = default;
    };
}

#endif //COLOR_HPP