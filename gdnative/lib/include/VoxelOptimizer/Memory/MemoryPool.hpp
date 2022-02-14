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

#ifndef MEMORYPOOL_HPP
#define MEMORYPOOL_HPP

#include <stddef.h>

namespace VoxelOptimizer
{
    template<class T, size_t GrowSize = 100>
    class CMemoryPool
    {
        public:
            using value_type = T;
            using size_type = size_t;
            using difference_type = std::ptrdiff_t;

            CMemoryPool();

            /**
             * @brief Allocates data of the pool.
             */
            T* allocate(size_type _n);

            /**
             * @brief Releases data to the pool.
             */
            void deallocate(T *_ptr, size_type _n);

            /**
             * @brief Frees all allocated blocks.
             */
            void clear();

            virtual ~CMemoryPool();

        private:
            struct Chunk
            {
                Chunk *Next;
            };

            class Block
            {
                public:
                    static const size_type ChunkSize;
                    Block(Block *_next, Chunk *_free, size_type _reserve);

                    Block *Next;
                    char *Data;

                    ~Block();
                private:
                    size_type m_Size;
            };

            void allocateBlock(size_type _size);

            Chunk *m_FirstFreeChunk;
            Block *m_Blocks;
    };

    //////////////////////////////////////////////////
    // CMemoryPool functions
    //////////////////////////////////////////////////

    template<class T, size_t GrowSize>
    inline CMemoryPool<T, GrowSize>::CMemoryPool() : m_FirstFreeChunk(nullptr), m_Blocks(nullptr) { }

    template<class T, size_t GrowSize>
    inline T* CMemoryPool<T, GrowSize>::allocate(size_type _n)
    {
        if(!m_FirstFreeChunk)
            allocateBlock(_n > GrowSize ? _n : GrowSize);

        Chunk *tmp = m_FirstFreeChunk;
        if(_n == 1)
        {
            m_FirstFreeChunk = tmp->Next;
            return (T*)tmp;
        }

        size_type continues = 0;
        while (tmp)
        {
            if((tmp + Block::ChunkSize) == tmp->Next)
                continues++;
            else
                continues = 0;

            if(continues == _n)
                break;

            tmp = tmp->Next;
        }
        
        if(continues == _n)
        {
            tmp = m_FirstFreeChunk;
            m_FirstFreeChunk = tmp->Next;
            return (T*)tmp;
        }
        
        allocateBlock(_n);
        tmp = m_FirstFreeChunk;
        m_FirstFreeChunk = tmp->Next;
        return (T*)tmp;
    }

    template<class T, size_t GrowSize>
    inline void CMemoryPool<T, GrowSize>::deallocate(T *_ptr, size_type _n)
    {
        for (size_t i = 0; i < _n; i++)
        {
            Chunk *tmp = (Chunk*)(_ptr + (Block::ChunkSize * i));
            tmp->Next = m_FirstFreeChunk;
            m_FirstFreeChunk = tmp;
        }
    }

    template<class T, size_t GrowSize>
    inline void CMemoryPool<T, GrowSize>::clear()
    {
        while(m_Blocks)
        {
            Block *tmp = m_Blocks->Next;
            delete m_Blocks;
            m_Blocks = tmp;
        }
        m_FirstFreeChunk = nullptr;
    }

    template<class T, size_t GrowSize>
    inline void CMemoryPool<T, GrowSize>::allocateBlock(size_type _size)
    {
        Block *tmp = new Block(m_Blocks, m_FirstFreeChunk, _size);
        m_Blocks = tmp;

        m_FirstFreeChunk = (Chunk*)tmp->Data;            
    }

    template<class T, size_t GrowSize>
    inline CMemoryPool<T, GrowSize>::~CMemoryPool()
    {
        clear();
    }

    //////////////////////////////////////////////////
    // CMemoryPool::Block functions
    //////////////////////////////////////////////////

    template<class T, size_t GrowSize>
    const typename CMemoryPool<T, GrowSize>::size_type CMemoryPool<T, GrowSize>::Block::ChunkSize = sizeof(T) >= sizeof(Chunk) ? sizeof(T) : sizeof(Chunk);

    template<class T, size_t GrowSize>
    inline CMemoryPool<T, GrowSize>::Block::Block(Block *_next, Chunk *_free, size_type _reserve) : Next(_next), m_Size(_reserve)
    {
        size_t tt = ChunkSize;

        Data = new char[ChunkSize * m_Size];
        Chunk *tmp = (Chunk*)Data;
        for (size_t i = 1; i < m_Size - 1; i++)
        {
            Chunk *next = (Chunk*)(Data + (i * ChunkSize));
            tmp->Next = next;
            tmp = next;
        }

        tmp->Next = _free;
    }

    template<class T, size_t GrowSize>
    inline CMemoryPool<T, GrowSize>::Block::~Block()
    {
        delete[] Data;
    }
} // namespace VoxelOptimizer

#endif