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
#include <new>

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
                    virtual void AddFreeStorage(CLocalStorage *_Storage) = 0;
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
                    CSegmentedMemory(void *_Block, uintptr_t _BlockSize, uint32_t _ChunkSize, void *_CustomData) : m_FreeChunks(nullptr) { SegmentateBlock(_Block, _BlockSize, _ChunkSize, _CustomData); }
                    
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
                     * @param _Mem: Memory to add back
                     */
                    inline void Deallocate(void *_Mem)
                    {
                        auto chunk = reinterpret_cast<Chunk*>(_Mem);
                        chunk->Next = m_FreeChunks;
                        m_FreeChunks = chunk;
                    }

                    /**
                     * @brief Segmentates a block of memory into multiple chunks.
                     * @param _Block: Continues block of memory to segmentate.
                     * @param _BlockSize: Size of the block.
                     * @param _ChunkSize: Size of one chunk.
                     */
                    inline void SegmentateBlock(void *_Block, uintptr_t _BlockSize, uint32_t _ChunkSize, void *_CustomData)
                    {
                        auto chunks = _BlockSize / _ChunkSize;

                        auto tmp = reinterpret_cast<Chunk*>(_Block);
                        m_FreeChunks = tmp;
                        for (size_t i = 1; i < chunks; i++)
                        {
                            tmp->CustomData = _CustomData;
                            tmp->Next = reinterpret_cast<Chunk*>(reinterpret_cast<std::byte*>(_Block) + (i * _ChunkSize));
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
                    CLocalStorage(uintptr_t _BlockSize, size_t _Chunksize) : m_Segments(reinterpret_cast<std::byte*>(this) + sizeof(m_Segments), _BlockSize, _Chunksize, this) {}

                    /** Allocates a new chunk of memory. */
                    inline void* Allocate() { return m_Segments.Allocate(); }

                    /** Deallocates a chunk of memory. */
                    inline void Deallocate(void *_Mem) { return m_Segments.Deallocate(_Mem); }

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

                    inline void SetStorage(IPool *_Pool, CLocalStorage *_Storage)
                    {
                        m_Pool = _Pool;
                        m_Storage = _Storage;
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
                inline void Deallocate(void *_Mem)
                {
                    // Each memory chunk has a small header which points back to it's storage.
                    auto chunkStart = reinterpret_cast<std::byte*>(_Mem) - _Internal::CSegmentedMemory::ChunkDataOffset;
                    auto storage = reinterpret_cast<_Internal::CLocalStorage*>(chunkStart);
                    bool full = storage->IsFull();
                    storage->Deallocate(chunkStart);

                    if(full)
                        m_FreeStorages.push_back(storage);

                    // CLocalStorage *storage = nullptr;
                    // auto memNumericPtr = reinterpret_cast<uintptr_t>(_Mem);

                    // for (auto &&block : m_Blocks)
                    // {
                    //     auto blockNumericPtr = reinterpret_cast<uintptr_t>(block);
                    //     if(memNumericPtr >= blockNumericPtr && memNumericPtr < (blockNumericPtr + BlockSize))
                    //     {
                    //         auto offset = (memNumericPtr - blockNumericPtr);
                    //         offset = (offset / (sizeof(void*) + LocalStorageSize));
                    //         storage = reinterpret_cast<CLocalStorage*>(reinterpret_cast<std::byte*>(block) + (offset * (sizeof(void*) + LocalStorageSize)));
                    //         break;
                    //     }
                    // }
                    
                    // bool full = storage->IsFull();
                    // storage->Deallocate(_Mem);

                    // if(full)
                    //     m_FreeStorages.push_back(storage);
                }

                template <class ...args>
                inline T* construct(args&& ..._args)
                {
                    T *obj = new(Allocate()) T(std::forward<args>(_args)...);
                    return obj;
                }

                inline void destruct(T* _ptr)
                {
                    _ptr->~T();
                    Deallocate(_ptr);
                }

                ~CObjectPool()
                {
                    _Internal::s_Storage.SetStorage(nullptr, nullptr);

                    // Frees all allocated memory.
                    for (auto &&block : m_Blocks)
                        delete[] block;
                }
            protected:
                void AddFreeStorage(_Internal::CLocalStorage *_Storage) override
                {
                    std::lock_guard<_Internal::LockFreeMutex> lock(m_Lock);
                    m_FreeStorages.push_back(_Storage);
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