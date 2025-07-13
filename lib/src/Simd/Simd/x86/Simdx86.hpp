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

#ifndef SIMDX86_HPP
#define SIMDX86_HPP

#undef __AVX2__

#if defined(__AVX2__)
    #include <immintrin.h>
#elif defined(__SSE2__)
    #include <emmintrin.h>
#endif

#include <cstring>

namespace VCore
{
    namespace Simd
    {
#if defined(__AVX2__)
        class NativeI
        {
            public:
                NativeI() : m_Value(_mm256_setzero_si256()) {}
                NativeI(const int _Value) : m_Value(_mm256_set1_epi32(_Value)) {}
                NativeI(const int *_Values, const unsigned char _Size) : NativeI() { Load(_Values, _Size); }
                NativeI(const NativeI &) = default;
                NativeI(NativeI &&) = default;

                /** Loads given values into the register. */
                inline NativeI &Load(const int *_Values, const unsigned char _Size)
                {
                    if(_Size < 8)
                    {
                        alignas(32) int buf[8] = {};
                        memcpy(buf, _Values, sizeof(int) * _Size);
                        m_Value = _mm256_load_si256((__m256i*)buf);
                    }
                    else
                        m_Value = _mm256_loadu_si256((__m256i*)_Values);

                    return *this;
                }

                inline void Store(int *_Values, const unsigned char _Size) const
                {
                    if(_Size < 8)
                    {
                        alignas(32) int buf[8] = {};
                        _mm256_store_si256((__m256i*)buf, m_Value);
                        memcpy(_Values, buf, sizeof(int) * _Size);
                    }
                    else
                        _mm256_storeu_si256((__m256i*)_Values, m_Value);
                }

                inline int MoveMask() const
                {
                    __m256i cmp = _mm256_cmpeq_epi32(m_Value, _mm256_setzero_si256());
                    return _mm256_test_epi32(cmp, cmp);
                }

                inline NativeI &operator=(const NativeI&) = default;
                inline NativeI &operator=(NativeI&&) = default;

                inline NativeI operator^(const NativeI &_Other) const
                {
                    return NativeI(_mm256_xor_si256(m_Value, _Other.m_Value));
                }
                
                inline NativeI operator&(const NativeI &_Other) const
                {
                    return NativeI(_mm256_and_si256(m_Value, _Other.m_Value));
                }
                
                inline NativeI operator==(const NativeI &_Other) const
                {
                    return NativeI(_mm256_cmpeq_epi32(m_Value, _Other.m_Value));
                }

            protected:
                NativeI(const __m256i &_Value) : m_Value(_Value) {}

            private:
                __m256i m_Value;
        };
#elif defined(__SSE2__)
        class NativeI
        {
            public:
                NativeI() : m_Value(_mm_setzero_si128()) {}
                NativeI(const int p_Value) : m_Value(_mm_set1_epi32(p_Value)) {}
                NativeI(const int *p_Values, const unsigned char p_Size) : NativeI() { Load(p_Values, p_Size); }
                NativeI(const NativeI &) = default;
                NativeI(NativeI &&) = default;

                /** Loads given values into the register. */
                inline NativeI &Load(const int *p_Values, const unsigned char p_Size)
                {
                    if(p_Size < 4)
                    {
                        alignas(32) int buf[4] = {};
                        memcpy(buf, p_Values, sizeof(int) * p_Size);
                        m_Value = _mm_load_si128((__m128i*)buf);
                    }
                    else
                        m_Value = _mm_loadu_si128((__m128i*)p_Values);

                    return *this;
                }

                inline void Store(int *p_Values, const unsigned char p_Size) const
                {
                    if(p_Size < 4)
                    {
                        alignas(32) int buf[4] = {};
                        _mm_store_si128((__m128i*)buf, m_Value);
                        memcpy(p_Values, buf, sizeof(int) * p_Size);
                    }
                    else
                        _mm_storeu_si128((__m128i*)p_Values, m_Value);
                }

                inline int MoveMask() const
                {
                    return _mm_movemask_ps(_mm_castsi128_ps(m_Value));
                }

                inline NativeI &operator=(const NativeI&) = default;
                inline NativeI &operator=(NativeI&&) = default;

                inline NativeI operator^(const NativeI &p_Other) const
                {
                    return NativeI(_mm_xor_si128(m_Value, p_Other.m_Value));
                };

                inline NativeI operator&(const NativeI &p_Other) const
                {
                    return NativeI(_mm_and_si128(m_Value, p_Other.m_Value));
                };

                inline NativeI operator==(const NativeI &p_Other) const
                {
                    return NativeI(_mm_cmpeq_epi32(m_Value, p_Other.m_Value));
                };

            protected:
                NativeI(const __m128i &p_Value) : m_Value(p_Value) {}

            private:
                __m128i m_Value;
        };
#endif
    } // namespace Simd
} // namespace VCore


#endif