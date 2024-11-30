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

#ifndef VPLATFORM_HPP
#define VPLATFORM_HPP

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

// Macros for the diffent architecture types.
#if defined(__x86_64__) || defined(_M_X64)
    #define VCORE_ARCH_x86_64
#elif defined(__i386__) || defined(_M_IX86)
    #define VCORE_ARCH_x86
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define VCORE_ARCH_ARM64
#elif defined(__arm__) || defined(_M_ARM)
    #define VCORE_ARCH_ARM
#else
    #define VCORE_ARCH_UNKNOWN
#endif

// Check for SIMD support.
#if defined(VCORE_ARCH_x86_64) || defined(VCORE_ARCH_x86)
    #if defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
        #define VCORE_SIMD_AVAILABLE
    #endif
#elif defined(VCORE_ARCH_ARM64) || defined(VCORE_ARCH_ARM)
    #if defined(__ARM_FEATURE_SIMD32) || defined(__ARM_NEON)
        #define VCORE_SIMD_AVAILABLE
    #endif
#endif

#endif