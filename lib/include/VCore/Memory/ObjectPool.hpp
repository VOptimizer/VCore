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

#ifndef POOL_HPP
#define POOL_HPP

#include <cstdint>
#include <cstddef>
#include <VCore/Misc/fast_vector.hpp>
#include <atomic>
#include <mutex>

#include <VCore/VPlatform.hpp>

#ifdef VCORE_ARCH_x86_64
    #ifdef VCORE_SIMD_AVAILABLE
        #include <immintrin.h>
    #endif
#endif

namespace VCore
{
    namespace Memory
    {
        namespace _Internal
        {
            class CLocalStorage;
            class CLocalStoragePointer;

            class IPool
            {
                friend CLocalStoragePointer;
                protected:
                    virtual void AddFreeStorage(CLocalStorage *p_Storage) = 0;
            };

            /** Lock-free mutex using std::atomic_flag */
            class LockFreeMutex
            {
                public:
                    LockFreeMutex() : m_Lock(ATOMIC_FLAG_INIT) {}
                    LockFreeMutex(LockFreeMutex &&) = delete;
                    LockFreeMutex(const LockFreeMutex &) = delete;

                    LockFreeMutex &operator=(LockFreeMutex &&) = delete;
                    LockFreeMutex &operator=(const LockFreeMutex &) = delete;

                    inline void lock()
                    {
                        while(m_Lock.test_and_set(std::memory_order_acquire))
                            _mm_pause();
                    }

                    inline void unlock()
                    {
                        m_Lock.clear(std::memory_order_release);
                    }
                private:
                    std::atomic_flag m_Lock;
            };

            /** Helper class to segmentate a block of memory in multiple chunks of the same size. */
            class CSegmentedMemory
            {
                public:
                    CSegmentedMemory() : m_FreeChunks(nullptr) {}

                    /** @see SegmentateBlock for more informations */
                    CSegmentedMemory(void *p_Block, uintptr_t p_BlockSize, uint32_t p_ChunkSize, void *p_CustomData) : m_FreeChunks(nullptr) { SegmentateBlock(p_Block, p_BlockSize, p_ChunkSize, p_CustomData); }
                    
                    /**
                     * @brief Allocates a new chunk of memory
                     * @return Returns the newly allocated chunk, or null, if there is no more chunks available.
                     */
                    inline void *Allocate()
                    {
                        auto result = m_FreeChunks;
                        if(result)
                        {
                            m_FreeChunks = result->Next;
                            result = reinterpret_cast<Chunk*>(reinterpret_cast<std::byte*>(result) + ChunkDataOffset);
                        }

                        return reinterpret_cast<void*>(result);
                    }

                    /**
                     * @brief Deallocates memory, and add it back to the pool.
                     * @param p_Mem: Memory to add back
                     */
                    inline void Deallocate(void *p_Mem)
                    {
                        auto chunk = reinterpret_cast<Chunk*>(p_Mem);
                        chunk->Next = m_FreeChunks;
                        m_FreeChunks = chunk;
                    }

                    /**
                     * @brief Segmentates a block of memory into multiple chunks.
                     * @param p_Block: Continues block of memory to segmentate.
                     * @param p_BlockSize: Size of the block.
                     * @param p_ChunkSize: Size of one chunk.
                     */
                    inline void SegmentateBlock(void *p_Block, uintptr_t p_BlockSize, uint32_t p_ChunkSize, void *p_CustomData)
                    {
                        auto chunks = p_BlockSize / p_ChunkSize;

                        auto tmp = reinterpret_cast<Chunk*>(p_Block);
                        m_FreeChunks = tmp;
                        for (size_t i = 1; i < chunks; i++)
                        {
                            tmp->CustomData = p_CustomData;
                            tmp->Next = reinterpret_cast<Chunk*>(reinterpret_cast<std::byte*>(p_Block) + (i * p_ChunkSize));
                            tmp = tmp->Next;
                        }
                        tmp->Next = nullptr;
                    }

                    /**
                     * @return Returns true, if no more chunks are left.
                     */
                    inline bool IsFull() const { return m_FreeChunks == nullptr; }
                private:
                    struct Chunk { void *CustomData; Chunk *Next; };

                    Chunk *m_FreeChunks;

                public:
                    static constexpr uint32_t ChunkHeaderSize = sizeof(Chunk);
                    static constexpr uint32_t ChunkDataOffset = offsetof(Chunk, Next);
            };

            /** Each block of memory is splitted into multiple storages, so that each thread can have it's own dedicated area. */
            class CLocalStorage
            {
                public:
                    CLocalStorage(uintptr_t p_BlockSize, size_t p_Chunksize) : m_Segments(reinterpret_cast<std::byte*>(this) + sizeof(m_Segments), p_BlockSize, p_Chunksize, this) {}

                    /** Allocates a new chunk of memory. */
                    inline void* Allocate() { return m_Segments.Allocate(); }

                    /** Deallocates a chunk of memory. */
                    inline void Deallocate(void *p_Mem) { return m_Segments.Deallocate(p_Mem); }

                    /** @return Returns true, if no more chunks are left. */
                    inline bool IsFull() const { return m_Segments.IsFull(); }
                private:
                    CSegmentedMemory m_Segments;
            };

            /** Similar to a shared_ptr just for thread_local */
            class CLocalStoragePointer
            {
                public:
                    CLocalStoragePointer() : m_Pool(nullptr), m_Storage(nullptr) {}
                    CLocalStoragePointer(const CLocalStoragePointer&) = delete;

                    inline void SetStorage(IPool *p_Pool, CLocalStorage *p_Storage)
                    {
                        m_Pool = p_Pool;
                        m_Storage = p_Storage;
                    }

                    inline CLocalStoragePointer &operator=(const CLocalStoragePointer&) = delete;

                    inline CLocalStorage *operator->() { return m_Storage; }

                    inline operator bool() { return m_Storage != nullptr; }

                    ~CLocalStoragePointer()
                    {
                        // Every none full storage will be added back to the pool.
                        if(m_Pool && m_Storage && !m_Storage->IsFull())
                            m_Pool->AddFreeStorage(m_Storage);
                    }
                private:
                    IPool *m_Pool;
                    CLocalStorage *m_Storage;
            };

            // There is an issue with mingw-w64 and redefinition of some internal function.
            // Also the constructor is called after free. This can be fixed by using
            // -static -static-libgcc -static-libstdc++ and statically link the standard libraries.
            // But this doesn't prevents gdb to not mess-up the destruction.
            // If you need mingw, please use LLVM-MinGW.
            // For more infos:
            // https://github.com/msys2/MINGW-packages/issues/2519#issuecomment-304155278
            inline static thread_local CLocalStoragePointer s_Storage;
        } // namespace _Internal
        
        /** 
         * @brief Thread-safe object pool
         * 
         * How does it works?
         * 
         * To make this memory pool thread-safe I use a combination of a thread_local variable
         * and arenas which are dedicated to each thread.
         * 
         * The pool allocated one big block of memory which can hold at least <Elements> elements. This block,
         * is than splitted into 10 arenas. Each thread gets one arena assigned and can allocate memory without
         * any mutex.
         */
        template <class T, size_t Elements = 100>
        class CObjectPool : public _Internal::IPool
        {
            public:
                CObjectPool() = default;

                /** Allocates a new chunk of memory. */
                inline void *Allocate()
                {
                    // Checks if the current thread has already an arena.
                    if(!_Internal::s_Storage)
                        AllocateStorage();

                    // Tries to allocate a chunk of memory from the arena.
                    // If there is no more chunks (Out of memory), a new arena will be allocated.
                    auto data = _Internal::s_Storage->Allocate();
                    if(!data)
                    {
                        AllocateStorage();
                        data = _Internal::s_Storage->Allocate();
                    }

                    return data;
                }

                /** Deallocates a chunk of memory. */
                inline void Deallocate(void *p_Mem)
                {
                    // Each memory chunk has a small header which points back to it's storage.
                    auto chunkStart = reinterpret_cast<std::byte*>(p_Mem) - _Internal::CSegmentedMemory::ChunkDataOffset;
                    auto storage = reinterpret_cast<_Internal::CLocalStorage*>(chunkStart);
                    bool full = storage->IsFull();
                    storage->Deallocate(chunkStart);

                    if(full)
                        m_FreeStorages.push_back(storage);
                }

                template <class ...args>
                inline T* construct(args&& ...p_args)
                {
                    T *obj = new(Allocate()) T(std::forward<args>(p_args)...);
                    return obj;
                }

                inline void destruct(T* p_ptr)
                {
                    p_ptr->~T();
                    Deallocate(p_ptr);
                }

                ~CObjectPool()
                {
                    _Internal::s_Storage.SetStorage(nullptr, nullptr);

                    // Frees all allocated memory.
                    for (auto &&block : m_Blocks)
                        delete[] block;
                }
            protected:
                void AddFreeStorage(_Internal::CLocalStorage *p_Storage) override
                {
                    std::lock_guard<_Internal::LockFreeMutex> lock(m_Lock);
                    m_FreeStorages.push_back(p_Storage);
                }

            private:
                static constexpr size_t ChunkSize = sizeof(T) >= (_Internal::CSegmentedMemory::ChunkHeaderSize - _Internal::CSegmentedMemory::ChunkDataOffset) ? (sizeof(T) + _Internal::CSegmentedMemory::ChunkDataOffset) : _Internal::CSegmentedMemory::ChunkHeaderSize;
                static constexpr size_t Storages = (Elements / 100);
                static constexpr size_t LocalStorageSize = ((Elements / Storages) * ChunkSize);
                static constexpr size_t BlockSize = Storages * (sizeof(_Internal::CLocalStorage) + LocalStorageSize);

                /**
                 * @brief Allocates a new storage from the calling thread or, if no memory is left, a whole new block or memory is allocated.
                 */
                inline void AllocateStorage()
                {
                    std::lock_guard<_Internal::LockFreeMutex> lock(m_Lock);

                    // Allocates a new block of memory, if no storages are left.
                    if(m_FreeStorages.size() == 0)
                    {
                        auto block = new std::byte[BlockSize];
                        m_Blocks.push_back(block);

                        for (size_t i = 0; i < Storages; i++)
                            m_FreeStorages.push_back(new(block + i * (sizeof(_Internal::CLocalStorage) + LocalStorageSize)) _Internal::CLocalStorage(LocalStorageSize, ChunkSize));
                    }

                    // Sets the current local storage of the thread, to one of the free ones.
                    _Internal::s_Storage.SetStorage(this, m_FreeStorages.pop_back());
                }

                // Raw memory blocks.
                fast_vector<std::byte*> m_Blocks;

                // A list of all free storages.
                fast_vector<_Internal::CLocalStorage*> m_FreeStorages;

                _Internal::LockFreeMutex m_Lock;
        };
    } // namespace Memory
} // namespace VCore

#endif