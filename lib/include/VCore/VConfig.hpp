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

namespace VCore
{
  namespace Config
  {
    // Type of the bitmask. This is also used to determine the size of the chunks.
    // Config: To adjust the size of the chunks, simply change the type.
    using bitmask_t = uint64_t;

    // Maximum value of the Bitmask_t
    static constexpr bitmask_t BitmaskMax = ~((bitmask_t)0);

    // ChunkSize is always the maximum bits of bitmask_t divided by two.
    static constexpr uint32_t ChunkSize = (sizeof(bitmask_t) * 8) >> 1;

    // Precomputed mask for later face determination.
    static constexpr bitmask_t FaceMask = ((bitmask_t)1 << ChunkSize) - 1;

    // Mask to convert a world space position to a inner chunk position.
    static constexpr uint32_t InnerChunkMask = Config::ChunkSize - 1;

    // Mask to convert a world space position to a chunk start position.
    static constexpr uint32_t ChunkPositionMask = ~InnerChunkMask;
  }
}

#endif