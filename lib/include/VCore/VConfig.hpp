/*
 * MIT License
 *
 * Copyright (c) 2023 Christian Tost
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

#ifndef VCONFIG_HPP
#define VCONFIG_HPP

// Checks if rtti is enabled, and sets an helper flag.
#if defined(__clang__)
  #if __has_feature(cxx_rtti)
    #define VCORE_RTTI_ENABLED
  #endif
#elif defined(__GNUC__)
  #if defined(__GXX_RTTI)
    #define VCORE_RTTI_ENABLED
  #endif
#elif defined(_MSC_VER)
  #if defined(_CPPRTTI)
    #define VCORE_RTTI_ENABLED
  #endif
#endif

// This will influence the size of the chunks.
#ifndef BITMASK_TYPE
#define BITMASK_TYPE uint64_t
#endif

// Chunksize if always maximum bits of BITMASK_TYPE divided by two.
#define CHUNK_SIZE ((sizeof(BITMASK_TYPE) * 8) >> 1)

// Precomputed mask for later face determination.
#define FACE_MASK (((BITMASK_TYPE)1 << CHUNK_SIZE) - 1)

#endif