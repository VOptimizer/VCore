#ifndef FAST_VECTOR_HPP
#define FAST_VECTOR_HPP

#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <utility>
#include <type_traits>

// #include <Windows.h>

#define custom_malloc(size) malloc(size) // HeapAlloc(GetProcessHeap(), 0, size)
#define custom_realloc(mem, size) realloc(mem, size) // HeapReAlloc(GetProcessHeap(), 0, mem, size)
#define custom_free(mem) free(mem) // HeapFree(GetProcessHeap(), 0, mem)

namespace VCore
{
    template <typename T, size_t DefaultSize = 10>
    class fast_vector
    {
        public:
            using const_iterator = T*;
            using iterator = T*;

            fast_vector() = default;
            fast_vector(fast_vector &&p_Other) noexcept { *this = std::move(p_Other); }
            fast_vector(const fast_vector &p_Other) { *this = p_Other; }
            fast_vector(size_t p_count, const T &p_Value)
            {
                reserve(p_count);
                m_Size = p_count;
                for (size_t i = 0; i < p_count; i++)
                    new(&m_Data[i]) T(p_Value);
            }

            fast_vector(std::initializer_list<T> p_l)
            {
                insert(end(), p_l.begin(), p_l.end());
            }

            inline void remove(const T &p_Value)
            {
                for (uint64_t i = 0; i < m_Size; i++) 
                {
                    if(m_Data[i] == p_Value)
                    {
                        m_Data[i].~T();
                        insert(m_Data + i, m_Data + i + 1, m_Data + m_Size);
                        m_Size--;
                        break;
                    }
                }
            }

            inline void push_back(const T &p_Value)
            {
                if(!m_Data)
                    reserve(DefaultSize);
                else if((m_Size + 1) >= m_Capacity)
                    reserve(m_Capacity * 1.5f + 1);

                new(&m_Data[m_Size++]) T(p_Value);
            }

            inline void push_back(T &&p_Value)
            {
                if(!m_Data)
                    reserve(DefaultSize);
                else if((m_Size + 1) >= m_Capacity)
                    reserve(m_Capacity * 1.5f + 1);

                new(&m_Data[m_Size++]) T(std::move(p_Value));
            }

            template<class ...Args>
            inline T& emplace_back(Args&& ...p_args)
            {
                if(!m_Data)
                    reserve(DefaultSize);
                else if((m_Size + 1) >= m_Capacity)
                    reserve(m_Capacity * 1.5f + 1);

                auto reference = new(&m_Data[m_Size++]) T(std::forward<Args>(p_args)...);
                return *reference;
            }

            inline T pop_back() { return m_Data[--m_Size]; }

            inline const_iterator begin() const { return m_Data; }
            inline const_iterator end() const { return m_Data + m_Size; }

            inline uint64_t size() const { return m_Size; }
            inline uint64_t capacity() const { return m_Capacity; }

            [[nodiscard]] bool empty() const { return m_Size == 0; }

            inline T &operator[](uint64_t p_idx) { return m_Data[p_idx]; }
            inline T &operator[](uint64_t p_idx) const { return m_Data[p_idx]; }
            
            inline T *data() const { return m_Data; }

            inline void insert(T* p_Position, const T* p_Begin, const T*p_End)
            {
                auto size = static_cast<uintptr_t>(p_End - p_Begin);
                auto from = static_cast<uintptr_t>(p_Position - begin());
                if((from + size) >= m_Capacity)
                    reserve(from + size);

                if constexpr(std::is_trivially_copyable_v<T>)
                    memcpy(m_Data + from, p_Begin, size * sizeof(T));
                else
                {
                    auto dataPtr = m_Data + from;
                    for (uint64_t i = 0; i < size; i++) 
                        new(&dataPtr[i]) T(p_Begin[i]);
                }
                    
                m_Size += size;
            }

            inline void reserve(size_t p_Size)
            {
                if(p_Size < m_Capacity)
                    return;

                m_Capacity += (p_Size - m_Capacity) + 1;
                m_Data = static_cast<T*>(custom_realloc(static_cast<void*>(m_Data), m_Capacity * sizeof(T)));
            }

            inline fast_vector &operator=(fast_vector &&p_Other) noexcept
            {
                m_Data = std::move(p_Other.m_Data);
                m_Size = std::move(p_Other.m_Size);
                m_Capacity = std::move(p_Other.m_Capacity);

                p_Other.m_Data = nullptr;
                p_Other.m_Size = 0;
                p_Other.m_Capacity = 0;

                return *this;
            }

            inline fast_vector &operator=(const fast_vector &p_Other)
            {
                m_Size = p_Other.m_Size;
                m_Capacity = p_Other.m_Capacity;

                m_Data = static_cast<T*>(custom_malloc(m_Capacity * sizeof(T)));
                for (size_t i = 0; i < m_Size; i++)
                    new(&m_Data[i]) T(p_Other.m_Data[i]);
                
                return *this;
            }

            inline void clear()
            {
                if(m_Data)
                {
                    for (uint64_t i = 0; i < m_Size; i++)
                        m_Data[i].~T();
                    
                    custom_free(static_cast<void*>(m_Data));

                    m_Data = nullptr;
                    m_Size = 0;
                    m_Capacity = 0;
                }
            }

            ~fast_vector() { clear(); }
        private:
            T *m_Data{};
            uint64_t m_Size{};
            uint64_t m_Capacity{};
    };
} // namespace VCore


#endif