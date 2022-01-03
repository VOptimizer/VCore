/*
 * MIT License
 *
 * Copyright (c) 2021 Christian Tost
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

#ifndef BINARYSTREAM_HPP
#define BINARYSTREAM_HPP

#include <algorithm>
#include <iterator>
#include <sstream>
#include <type_traits>
#include <vector>

namespace VoxelOptimizer
{
    class CBinaryStream
    {
        public:
            CBinaryStream() : m_Data(std::ios_base::in | std::ios_base::out | std::ios_base::binary) 
            {
                std::noskipws(m_Data);
            }

            template<class T, typename std::enable_if<!std::is_same<T, std::string>::value>::type* = nullptr>
            inline CBinaryStream &operator<<(T val)
            {
                m_Data.write((char*)&val, sizeof(T));
                return *this;
            }

            template<class T, typename std::enable_if<std::is_same<T, std::string>::value>::type* = nullptr>
            inline CBinaryStream &operator<<(const T &val)
            {
                uint32_t len = val.size();
                m_Data.write((char*)&len, sizeof(len));
                m_Data.write(val.data(), len);

                return *this;
            }

            inline void write(const char *data, uint32_t size)
            {
                m_Data.write(data, size);
            }

            std::vector<char> data()
            {
                m_Data.seekg(m_Data.beg);   
                std::vector<char> ret((std::istream_iterator<char>(m_Data)), std::istream_iterator<char>());     
                m_Data.seekg(m_Data.end);  

                return ret;
            }

            inline std::streampos offset()
            {
                m_Data.seekg(0, m_Data.end);
                return m_Data.tellg();
            }

            ~CBinaryStream() = default;

        private:
            std::stringstream m_Data;
    };
} // namespace VoxelOptimizer

#endif //BINARYSTREAM_HPP