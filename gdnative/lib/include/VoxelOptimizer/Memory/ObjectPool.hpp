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

#ifndef OBJECTPOOL_HPP
#define OBJECTPOOL_HPP

#include <stddef.h>
#include <VoxelOptimizer/Memory/MemoryPool.hpp>

namespace VoxelOptimizer
{
    template <class T>
    class CObjectPool
    {
        public:
            CObjectPool();
            CObjectPool(size_t _reserve);

            /**
             * @brief Allocates a new object of the pool.
             */
            template<class ...args>
            T* alloc(args&& ..._args);

            /**
             * @brief Releases an object to the pool.
             */
            void dealloc(T* _ptr);

            /**
             * @brief Reserves more objects.
             * @param _reserve: Size to allocate
             */
            void reserve(size_t _reserve);

            /**
             * @brief Deletes all objects.
             */
            void clear();

            ~CObjectPool();
        private:
            struct Object
            {
                uint8_t Data[sizeof(T)];
                Object *Prev;
                Object *Next;
            };

            void deleteList(Object *list);

            // All allocated objects in the pool
            Object *m_UsedObjects;

            // All free objects in the pool
            Object *m_FreeObjects;

            CMemoryPool<Object> m_Pool;
    };

    //////////////////////////////////////////////////
    // CObjectPool functions
    //////////////////////////////////////////////////

    template <class T>
    inline CObjectPool<T>::CObjectPool() : m_UsedObjects(nullptr), m_FreeObjects(nullptr) { }

    template <class T>
    inline CObjectPool<T>::CObjectPool(size_t _reserve) : CObjectPool()
    {
        reserve(_reserve);
    }

    template <class T>
    template <class ...args>
    inline T* CObjectPool<T>::alloc(args&& ..._args)
    {
        if(!m_FreeObjects)
            reserve(8);

        Object *ret = m_FreeObjects;
        m_FreeObjects = ret->Next;
        ret->Next = ret->Prev = nullptr;

        if(m_UsedObjects)
        {
            ret->Next = m_UsedObjects;
            m_UsedObjects->Prev = ret;
        }
        m_UsedObjects = ret;

        T *obj = new(ret->Data) T(std::forward<args>(_args)...);
        return obj; 
    }

    template <class T>
    inline void CObjectPool<T>::dealloc(T* _ptr)
    {
        _ptr->~T();
        Object *tmp = (Object*)_ptr;
        Object *next = tmp->Next, *prev = tmp->Prev;
        if(next)
            next->Prev = prev;

        if(prev)
            prev->Next = next;

        tmp->Next = tmp->Prev = nullptr;
        if(m_FreeObjects)
            tmp->Next = m_FreeObjects;
        m_FreeObjects = tmp;
    }

    template <class T>
    inline void CObjectPool<T>::reserve(size_t _reserve)
    {
        for (size_t i = 0; i < _reserve; i++)
        {
            Object *tmp = m_Pool.allocate(1); //new Object();
            tmp->Prev = tmp->Next = nullptr;

            if(m_FreeObjects)
                tmp->Next = m_FreeObjects;
            m_FreeObjects = tmp;
        }
    }

    template <class T>
    inline void CObjectPool<T>::clear()
    {
        deleteList(m_FreeObjects);
        deleteList(m_UsedObjects);

        m_FreeObjects = nullptr;
        m_UsedObjects = nullptr;
    }

    template <class T>
    inline void CObjectPool<T>::deleteList(Object *list)
    {
        while (list)
        {
            Object *tmp = list;
            list = list->Next;

            T *obj = (T*)tmp->Data;
            obj->~T();

            // delete tmp;
            m_Pool.deallocate(tmp, 1);
        }
    }

    template <class T>
    inline CObjectPool<T>::~CObjectPool()
    {
        clear();
    }

} // namespace VoxelOptimizer

#endif