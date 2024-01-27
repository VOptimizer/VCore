#include <xmmintrin.h>

#if defined(__GNUC__) && (__GNUC__ >= 4)
#define _VCORE_INLINE_ __attribute__((always_inline)) inline
#elif defined(__llvm__)
#define _VCORE_INLINE_ __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define _VCORE_INLINE_ __forceinline
#else
#define _VCORE_INLINE_ inline
#endif

// #define _VCORE_INLINE_ inline

namespace VCore
{
    _VCORE_INLINE_ void VectorAdd(const float _VecA[4], const float _VecB[4], float _Out[4])
    {
        auto veca = _mm_loadu_ps(_VecA);
        auto vecb = _mm_loadu_ps(_VecB);
        _mm_storeu_ps(_Out, _mm_add_ps(veca, vecb));
    }

    _VCORE_INLINE_ void VectorAdd(const int _VecA[4], const int _VecB[4], int _Out[4])
    {
        auto veca = _mm_loadu_si128((__m128i_u*)_VecA);
        auto vecb = _mm_loadu_si128((__m128i_u*)_VecB);
        _mm_storeu_si128((__m128i_u*)_Out, _mm_add_epi32(veca, vecb));
    }

    _VCORE_INLINE_ void VectorSub(const float _VecA[4], const float _VecB[4], float _Out[4])
    {
        auto veca = _mm_loadu_ps(_VecA);
        auto vecb = _mm_loadu_ps(_VecB);
        _mm_storeu_ps(_Out, _mm_sub_ps(veca, vecb));
    }

    _VCORE_INLINE_ void VectorSub(const int _VecA[4], const int _VecB[4], int _Out[4])
    {
        auto veca = _mm_loadu_si128((__m128i_u*)_VecA);
        auto vecb = _mm_loadu_si128((__m128i_u*)_VecB);
        _mm_storeu_si128((__m128i_u*)_Out, _mm_sub_epi32(veca, vecb));
    }

    _VCORE_INLINE_ int VectorEq(const float _VecA[4], const float _VecB[4])
    {
        auto veca = _mm_loadu_ps(_VecA);
        auto vecb = _mm_loadu_ps(_VecB);
        return _mm_movemask_ps(_mm_cmpeq_ps(veca, vecb)) == 0xF;
    }

    _VCORE_INLINE_ int VectorEq(const int _VecA[4], const int _VecB[4])
    {
        auto veca = _mm_loadu_si128((__m128i_u*)_VecA);
        auto vecb = _mm_loadu_si128((__m128i_u*)_VecB);
        return _mm_movemask_epi8(_mm_cmpeq_epi32(veca, vecb)) == 0xFFFF;
        // _mm_storeu_si128((__m128i_u*)_Out, _mm_sub_epi32(veca, vecb));
    }
} // namespace VCore
