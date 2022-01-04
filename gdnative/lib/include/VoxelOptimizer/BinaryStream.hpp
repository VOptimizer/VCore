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
#include <VoxelOptimizer/Vector.hpp>

namespace VoxelOptimizer
{
    class CBinaryStream
    {
        public:
            CBinaryStream() : m_Data(std::ios_base::in | std::ios_base::out | std::ios_base::binary) 
            {
                std::noskipws(m_Data);
            }

            CBinaryStream(const char *data, size_t size) : CBinaryStream()
            {
                m_Data.write(data, size);
            }

            CBinaryStream(const CBinaryStream &strm) : CBinaryStream() 
            {
                m_Data << strm.m_Data.rdbuf();
            }

            CBinaryStream &operator=(const CBinaryStream &strm)
            {
                m_Data.clear();
                m_Data.str("");
                m_Data << strm.m_Data.rdbuf();
                return *this;
            }

            template<class T, typename std::enable_if<!std::is_same<T, std::string>::value && !std::is_same<T, CVector>::value>::type* = nullptr>
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

            template<class T, typename std::enable_if<std::is_same<T, CVector>::value>::type* = nullptr>
            inline CBinaryStream &operator<<(const T &val)
            {
                m_Data.write((char*)val.v, sizeof(val.v));
                return *this;
            }

            template<class T, typename std::enable_if<!std::is_same<T, std::string>::value && !std::is_same<T, CVector>::value>::type* = nullptr>
            inline CBinaryStream &operator>>(T &val)
            {
                m_Data.read((char*)&val, sizeof(T));
                return *this;
            }

            template<class T, typename std::enable_if<std::is_same<T, std::string>::value>::type* = nullptr>
            inline CBinaryStream &operator>>(T &val)
            {
                uint32_t len = 0;
                m_Data.read((char*)&len, sizeof(len));

                val.resize(len);
                m_Data.read(&val[0], len);

                return *this;
            }

            template<class T, typename std::enable_if<std::is_same<T, CVector>::value>::type* = nullptr>
            inline CBinaryStream &operator>>(T &val)
            {
                m_Data.read((char*)val.v, sizeof(val.v));
                return *this;
            }

            inline void write(const char *data, uint32_t size)
            {
                m_Data.write(data, size);
            }

            inline void read(char *data, uint32_t size)
            {
                m_Data.read(data, size);
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
                return m_Data.tellg();
            }

            inline std::streampos size()
            {
                auto pos = offset();
                m_Data.seekg(0, m_Data.end);
                auto size = offset();
                m_Data.seekg(pos, m_Data.beg);
                return size;
            }

            inline void skip(std::streampos bytes)
            {
                m_Data.seekg(bytes, m_Data.cur);
            }

            inline void seek(std::streampos pos)
            {
                m_Data.seekg(pos, m_Data.beg);
            }

            inline void reset()
            {
                m_Data.seekg(0, m_Data.beg);
            }

            ~CBinaryStream() = default;

        private:
            std::stringstream m_Data;
    };
} // namespace VoxelOptimizer

#endif //BINARYSTREAM_HPP