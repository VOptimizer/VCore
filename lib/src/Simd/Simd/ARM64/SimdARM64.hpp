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

#ifndef SIMDARM64_HPP
#define SIMDARM64_HPP

#include <arm_neon.h>
#include <string.h>

namespace VCore
{
    namespace Simd
    {
        class NativeI
        {
            public:
                NativeI() : m_Value{} {}
                NativeI(const int _Value) : m_Value(vdupq_n_s32(_Value)) {}
                NativeI(const int *_Values, const unsigned char _Size) : NativeI() { Load(_Values, _Size); }
                NativeI(const NativeI &_Other) = default;
                NativeI(NativeI &&_Other) = default;

                inline NativeI &operator=(const NativeI &_Other) = default;

                inline NativeI &operator=(NativeI &&_Other) = default;
            
                /** Loads given values into the register. */
                inline NativeI &Load(const int *_Values, const unsigned char _Size)
                {
                    if (_Size < 4) 
                    {
                        alignas(16) int buf[4] = {};
                        memcpy(buf, _Values, sizeof(int) * _Size);
                
                        m_Value = vld1q_s32(buf);
                    } 
                    else
                        m_Value = vld1q_s32(_Values);

                    return *this;
                }

                inline void Store(int *_Values, const unsigned char _Size) const
                {
                    if(_Size < 4)
                    {
                        alignas(16) int buf[4] = {};
                        vst1q_s32(buf, m_Value);
                        memcpy(_Values, buf, sizeof(int) * _Size);
                    }
                    else
                        vst1q_s32(_Values, m_Value);
                }

                inline int MoveMask() const
                {
                    alignas(16) int buf[4] = {};
                    Store(buf, 4);
                    return ((buf[0] >> (sizeof(int) * 8 - 1)) & 0x1) | (((buf[1] >> (sizeof(int) * 8 - 1)) & 0x1) << 1) | (((buf[2] >> (sizeof(int) * 8 - 1)) & 0x1) << 2) | (((buf[3] >> (sizeof(int) * 8 - 1)) & 0x1) << 3);
                }

                inline NativeI operator^(const NativeI &_Other) const
                {
                    return NativeI(veorq_s32(m_Value, _Other.m_Value));
                };

                inline NativeI operator&(const NativeI &_Other) const
                {
                    return NativeI(vandq_s32(m_Value, _Other.m_Value));
                };

                inline NativeI operator==(const NativeI &_Other) const
                {
                    return NativeI(vceqq_s32(m_Value, _Other.m_Value));
                };

            protected:
                NativeI(const int32x4_t &_Value) : m_Value(_Value) {}

            private:
                int32x4_t m_Value;
        };
    } // namespace Simd
} // namespace VCore

#endif