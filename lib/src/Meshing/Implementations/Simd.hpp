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

#ifndef SIMD_HPP
#define SIMD_HPP

#include <VCore/VPlatform.hpp>
#include <string.h>

#if (defined(VCORE_ARCH_x86_64) || defined(VCORE_ARCH_x86)) && defined(VCORE_SIMD_AVAILABLE)
#include <immintrin.h>
#endif

namespace VCore
{
    namespace Simd
    {
        template <int Bits>
        class Simdi { };

        template <>
        class Simdi<128> 
        {
            public:
                Simdi() : m_Value(_mm_setzero_si128()) {}
                Simdi(__m128i _Value) : m_Value(_Value) {}
                Simdi(const int _Value) : m_Value(_mm_set1_epi32(_Value)) {}
                Simdi(const int *_Values, const unsigned char _Size) : Simdi() { Load(_Values, _Size); }

                Simdi(const Simdi&) = default;
                Simdi(Simdi&&) = default;

                /** Loads given values into the register. */
                inline Simdi &Load(const int *_Values, const unsigned char _Size)
                {
                    if(_Size < 4)
                    {
                        alignas(16) int buf[4] = {};
                        memcpy(buf, _Values, sizeof(int) * _Size);
                        m_Value = _mm_load_si128((__m128i*)buf);
                    }
                    else
                        m_Value = _mm_loadu_si128((__m128i*)_Values);

                    return *this;
                }

                inline void Store(int *_Values, const unsigned char _Size) const
                {
                    if(_Size < 4)
                    {
                        alignas(16) int buf[4] = {};
                        _mm_store_si128((__m128i*)buf, m_Value);
                        memcpy(_Values, buf, sizeof(int) * _Size);
                    }
                    else
                        _mm_storeu_si128((__m128i*)_Values, m_Value);
                }

                inline int MoveMask() const
                {
                    return _mm_movemask_ps(_mm_castsi128_ps(m_Value));
                }

                inline Simdi &operator=(const Simdi&) = default;
                inline Simdi &operator=(Simdi&&) = default;

                inline Simdi operator^(const Simdi &_Other) const
                {
                    return Simdi(_mm_xor_si128(m_Value, _Other.m_Value));
                };

                inline Simdi operator&(const Simdi &_Other) const
                {
                    return Simdi(_mm_and_si128(m_Value, _Other.m_Value));
                };

                inline Simdi operator==(const Simdi &_Other) const
                {
                    return Simdi(_mm_cmpeq_epi32(m_Value, _Other.m_Value));
                };
            private:
                __m128i m_Value;
        };
    
        template <>
        class Simdi<256> 
        {
            public:
                Simdi() : m_Value(_mm256_setzero_si256()) {}
                Simdi(const __m256i &_Value) : m_Value(_Value) {}
                Simdi(const int _Value) : m_Value(_mm256_set1_epi32(_Value)) {}
                Simdi(const int *_Values, const unsigned char _Size) : Simdi() { Load(_Values, _Size); }

                Simdi(const Simdi&) = default;
                Simdi(Simdi&&) = default;

                /** Loads given values into the register. */
                inline Simdi &Load(const int *_Values, const unsigned char _Size)
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
                    return _mm256_movemask_ps(_mm256_castsi256_ps(m_Value));
                }

                inline Simdi &operator=(const Simdi&) = default;
                inline Simdi &operator=(Simdi&&) = default;

                inline Simdi operator^(const Simdi &_Other) const
                {
                    return Simdi(_mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(m_Value), _mm256_castsi256_ps(_Other.m_Value))));
                };

                inline Simdi operator&(const Simdi &_Other) const
                {
                    return Simdi(_mm256_castps_si256(_mm256_and_ps(_mm256_castsi256_ps(m_Value), _mm256_castsi256_ps(_Other.m_Value))));
                };

                inline Simdi operator==(const Simdi &_Other) const
                {
                    return Simdi(_mm256_castps_si256(_mm256_cmp_ps(_mm256_castsi256_ps(m_Value), _mm256_castsi256_ps(_Other.m_Value), _CMP_EQ_OQ)));
                };
            private:
                __m256i m_Value;
        };
    } // namespace simd
} // namespace VCore

#endif