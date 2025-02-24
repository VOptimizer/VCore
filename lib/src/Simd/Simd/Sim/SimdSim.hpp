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

#ifndef SIMDSIM_HPP
#define SIMDSIM_HPP

#include <string.h>
#include <utility>

namespace VCore
{
    namespace Simd
    {
        /** 
         * Fallback for platforms with no SSE or without implementation.
         * This implementation "simulates" a type which is similar to __m128i on x86
         */
        class NativeI
        {
            public:
                NativeI() : m_Value{} {}
                NativeI(const int _Value) { for(unsigned i = 0; i < sizeof(m_Value) / sizeof(m_Value[0]); i++) m_Value[i] = _Value; }
                NativeI(const int *_Values, const unsigned char _Size) : NativeI() { Load(_Values, _Size); }
                NativeI(const NativeI &_Other) { *this = _Other; }
                NativeI(NativeI &&_Other) { *this = std::move(_Other); }

                inline NativeI &operator=(const NativeI &_Other)
                {
                    memcpy(m_Value, _Other.m_Value, sizeof(m_Value));
                    return *this;
                }

                inline NativeI &operator=(NativeI &&_Other)
                {
                    memcpy(m_Value, _Other.m_Value, sizeof(m_Value));
                    memset(_Other.m_Value, 0, sizeof(_Other.m_Value));
                    return *this;
                }
            
                /** Loads given values into the register. */
                inline NativeI &Load(const int *_Values, const unsigned char _Size)
                {
                    if(_Size < 8)
                    {
                        // alignas(32) int buf[8] = {};
                        memset(m_Value, 0, sizeof(m_Value));
                        memcpy(m_Value, _Values, sizeof(int) * _Size);
                    }
                    else
                        memcpy(m_Value, _Values, sizeof(m_Value));

                    return *this;
                }

                inline void Store(int *_Values, const unsigned char _Size) const
                {
                    memcpy(_Values, m_Value, _Size < 8 ? _Size : sizeof(m_Value));
                }

                inline int MoveMask() const
                {
                    int mask = 0;
                    for (size_t i = 0; i < sizeof(m_Value) / sizeof(m_Value[0]); i++)
                        mask |= ((m_Value[i] >> (sizeof(int) * 8 - 1)) & 1) << i;

                    return mask;
                }

                inline NativeI operator^(const NativeI &_Other) const
                {
                    NativeI result;
                    for (size_t i = 0; i < sizeof(m_Value) / sizeof(m_Value[0]); i++)
                        result.m_Value[i] = m_Value[i] ^ _Other.m_Value[i];

                    return result;
                };

                inline NativeI operator&(const NativeI &_Other) const
                {
                    NativeI result;
                    for (size_t i = 0; i < sizeof(m_Value) / sizeof(m_Value[0]); i++)
                        result.m_Value[i] = m_Value[i] & _Other.m_Value[i];

                    return result;
                };

                inline NativeI operator==(const NativeI &_Other) const
                {
                    NativeI result;
                    for (size_t i = 0; i < sizeof(m_Value) / sizeof(m_Value[0]); i++)
                        result.m_Value[i] = m_Value[i] == _Other.m_Value[i];

                    return result;
                };

            private:
                int m_Value[4];
        };
    } // namespace Simd
} // namespace VCore


#endif