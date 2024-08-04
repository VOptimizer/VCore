#ifndef FAST_VECTOR_HPP
#define FAST_VECTOR_HPP

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

namespace VCore
{
    template <typename T>
    class fast_vector
    {
        public:
            fast_vector() : m_Data(nullptr), m_Size(0), m_Capacity(0) {}
            fast_vector(fast_vector &&_Other) { *this = std::move(_Other); }
            fast_vector(const fast_vector &_Other) { *this = _Other; }

            inline void push_back(const T &_Value)
            {
                if(!m_Data)
                {
                    m_Capacity = 100;
                    m_Data = static_cast<T*>(malloc(m_Capacity * sizeof(T)));
                }
                else if((m_Size + 1) >= m_Capacity)
                {
                    m_Capacity *= 1.5f;
                    m_Data = static_cast<T*>(realloc(static_cast<void*>(m_Data), m_Capacity * sizeof(T)));
                }

                new(&m_Data[m_Size++]) T(_Value);
            }

            inline T* begin() const { return m_Data; }
            inline T* end() const { return m_Data + m_Size; }

            inline uint64_t size() const { return m_Size; }
            inline uint64_t capacity() const { return m_Capacity; }

            inline T &operator[](uint64_t _idx)
            {
                return m_Data[_idx];
            }

            inline T &operator[](uint64_t _idx) const
            {
                return m_Data[_idx];
            }

            inline T *data() const { return m_Data; }

            inline fast_vector &operator=(fast_vector &&_Other)
            {
                m_Data = std::move(_Other.m_Data);
                m_Size = std::move(_Other.m_Size);
                m_Capacity = std::move(_Other.m_Capacity);

                _Other.m_Data = nullptr;
                _Other.m_Size = 0;
                _Other.m_Capacity = 0;

                return *this;
            }

            inline fast_vector &operator=(const fast_vector &_Other)
            {
                m_Size = _Other.m_Size;
                m_Capacity = _Other.m_Capacity;

                m_Data = static_cast<T*>(malloc(m_Capacity * sizeof(T)));
                for (size_t i = 0; i < m_Size; i++)
                    new(&m_Data[i]) T(_Other.m_Data[i]);
                
                return *this;
            }

            inline void clear()
            {
                if(m_Data)
                {
                    for (uint64_t i = 0; i < m_Size; i++)
                        m_Data[i].~T();
                    
                    free(static_cast<void*>(m_Data));

                    m_Data = nullptr;
                    m_Size = 0;
                    m_Capacity = 0;
                }
            }

            ~fast_vector()
            {
                clear();
            }
        private:
            T *m_Data;
            uint64_t m_Size;
            uint64_t m_Capacity;
    };
} // namespace VCore


#endif