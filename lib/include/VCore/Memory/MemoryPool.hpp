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

#include <assert.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <new>
#include <stddef.h>

namespace VCore
{
    static constexpr uintptr_t AlignTo = sizeof(uintptr_t) - 1;
    static constexpr uintptr_t PointerMask = ~AlignTo;

    template<class T, size_t BlockSize = 100>
    class CMemoryPool
    {
        public:
            using value_type = T;
            using size_type = size_t;
            using difference_type = std::ptrdiff_t;

            CMemoryPool() : m_FirstFreeChunk(TaggedPointer()), m_Blocks(nullptr) { }
            CMemoryPool(CMemoryPool &&) = default;
            CMemoryPool(const CMemoryPool &) = delete;

            CMemoryPool &operator=(CMemoryPool &&) = default;
            CMemoryPool &operator=(const CMemoryPool &) = delete;

            /**
             * @brief Constructs a new object from the pool.
             */
            template<class ...args>
            T* construct(args&& ..._args);

            /**
             * @brief Releases an object to the pool.
             */
            void destruct(T* _ptr);

            /**
             * @brief Allocates data of the pool, without intializing it.
             */
            T* allocate(size_type _n);

            /**
             * @brief Releases data to the pool. Without deinitializing it.
             */
            void deallocate(T *_ptr, size_type);

            /**
             * @brief Frees all allocated blocks.
             */
            void clear();

            virtual ~CMemoryPool() { clear(); }

        private:
            struct Chunk { Chunk *Next; };
            static constexpr size_type ChunkSize = sizeof(T) >= sizeof(Chunk) ? sizeof(T) : sizeof(Chunk);

            union TaggedPointer
            {
                TaggedPointer() : Ptr(nullptr) {}

                Chunk *Ptr;
                uintptr_t Bits;
            };

            class Block
            {
                public:
                    Block(Block *_next, Chunk *_free);

                    char Data[BlockSize * CMemoryPool::ChunkSize];
                    Block *Next;

                    ~Block() = default;
            };

            void AllocateBlock();

            // std::atomic<Chunk*> m_FirstFreeChunk;
            std::atomic<TaggedPointer> m_FirstFreeChunk;
            Block* m_Blocks;

            // Locks the allocation of a new block.
            std::mutex m_BlockLock;
    };

    //////////////////////////////////////////////////
    // CMemoryPool functions
    //////////////////////////////////////////////////

    template <class T, size_t BlockSize>
    template <class ...args>
    inline T* CMemoryPool<T, BlockSize>::construct(args&& ..._args)
    {
        auto data = allocate(sizeof(T));
        T *obj = new(data) T(std::forward<args>(_args)...);
        return obj; 
    }

    template <class T, size_t BlockSize>
    inline void CMemoryPool<T, BlockSize>::destruct(T* _ptr)
    {
        _ptr->~T();
        deallocate(_ptr, sizeof(T));
    }

    template<class T, size_t BlockSize>
    inline T* CMemoryPool<T, BlockSize>::allocate(size_type _n)
    {
        if(sizeof(T) != _n)
            throw std::bad_alloc();

        TaggedPointer tagged = m_FirstFreeChunk.load(std::memory_order_acquire);
        Chunk *tmp = nullptr; //tagged.Ptr & PointerMask;
        Chunk *next = nullptr;

        TaggedPointer newptr;
        do
        {
            tmp = (Chunk*)(tagged.Bits & PointerMask);

            if(tmp)
                next = tmp->Next;
            else
            {
                // If a new block of memory need to be allocated, we need every thread to wait, which tries to
                // allocate a new block. Otherwise, each thread will create a new empty block of memory, which isn't
                // be used at worst.
                std::lock_guard<std::mutex> lock(m_BlockLock);
                tagged = m_FirstFreeChunk.load(std::memory_order_acquire);
                if(!(tagged.Bits & PointerMask))
                    AllocateBlock();

                tagged = m_FirstFreeChunk.load(std::memory_order_acquire);
                tmp = (Chunk*)(tagged.Bits & PointerMask);
                if(tmp)
                    next = tmp->Next;
                else
                    next = nullptr;
            }

            auto tag = (tagged.Bits & AlignTo);
            tag++;
            if(tag > 7)
                tag = 0;

            newptr.Ptr = next;
            newptr.Bits |= tag;
        } while(!m_FirstFreeChunk.compare_exchange_weak(tagged, newptr, std::memory_order_release, std::memory_order_acquire));

        return reinterpret_cast<T*>(tmp);
    }

    template<class T, size_t BlockSize>
    inline void CMemoryPool<T, BlockSize>::deallocate(T *_ptr, size_type)
    {
        TaggedPointer tagged;

        auto tmp = (Chunk*)_ptr;
        tagged.Ptr = tmp;
        tagged.Bits |= 1;

        TaggedPointer head = m_FirstFreeChunk.load(std::memory_order_acquire);
        do
        {
            tmp->Next = (Chunk*)(head.Bits & PointerMask);
        } while (!m_FirstFreeChunk.compare_exchange_weak(head, tagged, std::memory_order_release, std::memory_order_acquire));
    }

    template<class T, size_t BlockSize>
    inline void CMemoryPool<T, BlockSize>::clear()
    {
        std::lock_guard<std::mutex> lock(m_BlockLock);
        while(m_Blocks)
        {
            Block *tmp = m_Blocks->Next;
            delete m_Blocks;
            m_Blocks = tmp;
        }
        m_FirstFreeChunk = TaggedPointer();
    }

    template<class T, size_t BlockSize>
    inline void CMemoryPool<T, BlockSize>::AllocateBlock()
    {
        TaggedPointer tagged = m_FirstFreeChunk.load(std::memory_order_relaxed);
        Block *tmp = new Block(m_Blocks, (Chunk*)(tagged.Bits & PointerMask));
        m_Blocks = tmp;

        tagged.Ptr = (Chunk*)tmp->Data;
        assert((reinterpret_cast<uintptr_t>(tagged.Ptr) & AlignTo) == 0);

        tagged.Bits |= 0;
        m_FirstFreeChunk = tagged;            
    }

    //////////////////////////////////////////////////
    // CMemoryPool::Block functions
    //////////////////////////////////////////////////

    template<class T, size_t BlockSize>
    inline CMemoryPool<T, BlockSize>::Block::Block(Block *_next, Chunk *_free) : Next(_next)
    {
        Chunk *tmp = (Chunk*)Data;
        for (size_t i = 1; i < BlockSize - 1; i++)
        {
            Chunk *next = (Chunk*)(Data + (i * CMemoryPool<T, BlockSize>::ChunkSize));
            tmp->Next = next;
            tmp = next;
        }

        tmp->Next = _free;
    }
} // namespace VoxelOptimizer

#endif