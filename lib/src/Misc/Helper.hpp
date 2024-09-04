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

#ifndef STRING_HPP
#define STRING_HPP

#include <string>
#include <vector>
#include <climits>
#include <VCore/VConfig.hpp>

#if defined(__GNUC__) || defined(__clang__)
    #if defined(__has_builtin)
        #if __has_builtin(__builtin_ctz)
            #define VCORE_HAS_BUILTIN_CTZ 1
        #else
            #define VCORE_HAS_BUILTIN_CTZ 0
        #endif
    #else
        #define VCORE_HAS_BUILTIN_CTZ 1
    #endif
#else
    #ifdef _MSC_VER
        #include <intrin.h>
    #endif

    #define VCORE_HAS_BUILTIN_CTZ 0
#endif

namespace VCore
{
    inline std::vector<std::string> split(const std::string &_Str, const std::string &_Delimiter)
    {
        std::vector<std::string> result;

        size_t pos = 0;
        std::string token;
        size_t latestPos = pos;
        while ((pos = _Str.find(_Delimiter, pos)) != std::string::npos) 
        {
            token = _Str.substr(0, pos);
            result.push_back(token);
            pos += _Delimiter.length();
            latestPos = pos;
        }

        result.push_back(_Str.substr(latestPos));

        return result;
    }

    template<class T>
    inline unsigned int CountTrailingZeroBitsFallback(T _Bits)
    {
        unsigned int count = 0;
        while ((_Bits & 1) == 0) 
        {
            _Bits >>= 1;
            count++;
        }

        return count;
    }

    template<class T>
    inline unsigned int CountTrailingZeroBits(T _Bits)
    {
        if (_Bits == 0) 
            return sizeof(_Bits) * CHAR_BIT;

        return CountTrailingZeroBitsFallback(_Bits);
    }

    template<>
    inline unsigned int CountTrailingZeroBits(uint32_t _Bits)
    {
        if (_Bits == 0) 
            return sizeof(_Bits) * CHAR_BIT;

        // First check for compiler support for this functionality.
        // Since the compiler can use CPU instructions which are far more faster than a loop.
        #ifdef VCORE_HAS_BUILTIN_CTZ
            return static_cast<unsigned int>(__builtin_ctz(_Bits));
        #elif defined(_MSC_VER)
            unsigned long index;
            _BitScanForward(&index, _Bits);
            return static_cast<unsigned int>(index);
        #else
            return CountTrailingZeroBitsFallback(_Bits);
        #endif
    }

    template<>
    inline unsigned int CountTrailingZeroBits(uint64_t _Bits)
    {
        if (_Bits == 0) 
            return sizeof(_Bits) * CHAR_BIT;

        // First check for compiler support for this functionality.
        // Since the compiler can use CPU instructions which are far more faster than a loop.
        #ifdef VCORE_HAS_BUILTIN_CTZ
            #if (LLONG_MAX != 9223372036854775807LL) && (LONG_MAX != 9223372036854775807L)
                #error "No 64 bits type support"
            #elif LLONG_MAX == 9223372036854775807LL
                return static_cast<unsigned int>(__builtin_ctzll(_Bits));
            #else
                return static_cast<unsigned int>(__builtin_ctzl(_Bits));
            #endif
        #elif defined(_MSC_VER)
            unsigned long index;
            _BitScanForward64(&index, _Bits);
            return static_cast<unsigned int>(index);
        #else
            return CountTrailingZeroBitsFallback(_Bits);
        #endif
    }

    // /**
    //  * @brief Counts the number of trailing zeros and returns their count.
    //  */
    // inline unsigned int CountTrailingZeroBits(BITMASK_TYPE _Bits)
    // {
    //     if (_Bits == 0) 
    //         return sizeof(_Bits) * CHAR_BIT;

    //     // First check for compiler support for this functionality.
    //     // Since the compiler can use CPU instructions which are far more faster than a loop.
    //     #ifdef VCORE_HAS_BUILTIN_CTZ
    //         return static_cast<unsigned int>(__builtin_ctz(_Bits));
    //     #elif defined(_MSC_VER)
    //         unsigned long index;
    //         _BitScanForward(&index, _Bits);
    //         return static_cast<unsigned int>(index);
    //     #else
    //         unsigned int count = 0;
    //         while ((_Bits & 1) == 0) 
    //         {
    //             _Bits >>= 1;
    //             count++;
    //         }

    //         return count;
    //     #endif
    // }

    /**
     * @brief Counts the number of trailing ones and returns their count.
     */
    inline unsigned int CountTrailingOneBits(BITMASK_TYPE _Bits)
    {
        return CountTrailingZeroBits(~_Bits);
    }
} // namespace VCore


#endif