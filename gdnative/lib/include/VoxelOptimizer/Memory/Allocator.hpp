/*
* MIT License
*
* Copyright (c) 2022 Christian Tost
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

#ifndef ALLOCATOR_HPP
#define ALLOCATOR_HPP

#include <VoxelOptimizer/Memory/MemoryPool.hpp>

namespace VoxelOptimizer
{
    template<class T>
    class CAllocator : public CMemoryPool<T>
    {
        public:
            using value_type = typename CMemoryPool<T>::value_type;
            using size_type = typename CMemoryPool<T>::size_type;
            using difference_type = typename CMemoryPool<T>::difference_type;
            using pointer = T*;
            using const_pointer = const T*;
            using reference = T&;
            using const_reference = const T&;

            template<class U> struct rebind 
            {
                typedef CAllocator<U> other;
            };

            CAllocator();

            pointer allocate( size_type n, const void * hint = 0 );

            ~CAllocator();
    };

    //////////////////////////////////////////////////
    // CMemoryPool functions
    //////////////////////////////////////////////////

    template< class T1, class T2 >
    inline bool operator==(const CAllocator<T1>& lhs, const CAllocator<T2>& rhs)
    {
        return true;
    }

    template< class T1, class T2 >
    inline bool operator!=(const CAllocator<T1>& lhs, const CAllocator<T2>& rhs)
    {
        return false;
    }

    template<class T>
    inline CAllocator<T>::CAllocator() : CMemoryPool<T>() { }

    template<class T>
    inline CAllocator<T>::~CAllocator() { }

    template<class T>
    inline typename CAllocator<T>::pointer CAllocator<T>::allocate(size_type n, const void * hint)
    {
        return CMemoryPool<T>::allocate(n);
    }
} // namespace VoxelOptimizer


#endif