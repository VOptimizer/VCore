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
#include <cstdio>
#include <cstring>
#include <iterator>
#include <string>
#include <type_traits>
#include <vector>
#include <VoxelOptimizer/Math/Vector.hpp>

namespace VoxelOptimizer
{
    template<size_t BlockSize = 4096>
    class CBufferedStream
    {
        public:
            CBufferedStream();
            CBufferedStream(const char *_data, size_t _size);
            CBufferedStream(const CBufferedStream &_strm);
            CBufferedStream(const std::string &_filename);

            CBufferedStream &write(const char *_data, size_t _size);
            size_t read(char *_data, size_t _size);

            std::vector<char> data();
            size_t offset();
            size_t size();
            void skip(size_t _bytes);
            void reset();
            void load(const std::string &_filename);

            CBufferedStream &operator=(const CBufferedStream &strm);

            virtual ~CBufferedStream();
        private:
            struct SBlock
            {
                public:
                    SBlock();
                    SBlock(const SBlock &_block);

                    size_t write(const char *_data, size_t _size, size_t _offset);
                    size_t read(char *_data, size_t _size, size_t _offset);

                    size_t Size;
                    char Data[BlockSize];
            };

            void clearData();
            void copyStream(const CBufferedStream &_strm);
            void allocateBlocks(size_t _blocks);

            std::vector<SBlock*> m_Blocks;
            size_t m_Size;
            size_t m_Offset;
    };

    class CBinaryStream : public CBufferedStream<4096>
    {
        public:
            CBinaryStream() = default;
            CBinaryStream(const char *_data, size_t _size) : CBufferedStream<4096>(_data, _size) {}
            CBinaryStream(const CBufferedStream &_strm) : CBufferedStream<4096>(_strm) {}
            CBinaryStream(const std::string &_filename) : CBufferedStream<4096>(_filename) {}

            template<class T, typename std::enable_if<!std::is_same<T, std::string>::value && !std::is_same<T, CVector>::value>::type* = nullptr>
            inline CBinaryStream &operator<<(T val)
            {
                write((char*)&val, sizeof(T));
                return *this;
            }

            template<class T, typename std::enable_if<std::is_same<T, std::string>::value>::type* = nullptr>
            inline CBinaryStream &operator<<(const T &val)
            {
                uint32_t len = val.size();
                write((char*)&len, sizeof(len));
                write(val.data(), len);

                return *this;
            }

            template<class T, typename std::enable_if<std::is_same<T, CVector>::value>::type* = nullptr>
            inline CBinaryStream &operator<<(const T &val)
            {
                write((char*)val.v, sizeof(val.v));
                return *this;
            }

            // template<class T, typename std::enable_if<std::is_same<T, CVectori>::value>::type* = nullptr>
            // inline CBinaryStream &operator<<(const T &val)
            // {
            //     write((char*)val.v, sizeof(val.v));
            //     return *this;
            // }

            template<class T, typename std::enable_if<!std::is_same<T, std::string>::value && !std::is_same<T, CVector>::value>::type* = nullptr>
            inline CBinaryStream &operator>>(T &val)
            {
                read((char*)&val, sizeof(T));
                return *this;
            }

            template<class T, typename std::enable_if<std::is_same<T, std::string>::value>::type* = nullptr>
            inline CBinaryStream &operator>>(T &val)
            {
                uint32_t len = 0;
                read((char*)&len, sizeof(len));

                val.resize(len);
                read(&val[0], len);

                return *this;
            }

            template<class T, typename std::enable_if<std::is_same<T, CVector>::value>::type* = nullptr>
            inline CBinaryStream &operator>>(T &val)
            {
                read((char*)val.v, sizeof(val.v));
                return *this;
            }

            // template<class T, typename std::enable_if<std::is_same<T, CVectori>::value>::type* = nullptr>
            // inline CBinaryStream &operator>>(T &val)
            // {
            //     read((char*)val.v, sizeof(val.v));
            //     return *this;
            // }

            ~CBinaryStream() = default;
    };

    //////////////////////////////////////////////////
    // CBufferedStream functions
    //////////////////////////////////////////////////

    template<size_t BlockSize>
    inline CBufferedStream<BlockSize>::CBufferedStream() : m_Size(0), m_Offset(0)
    {
        allocateBlocks(1);
    }

    template<size_t BlockSize>
    inline CBufferedStream<BlockSize>::CBufferedStream(const char *_data, size_t _size) : CBufferedStream() 
    {
        write(_data, _size);
        reset();
    }

    template<size_t BlockSize>
    inline CBufferedStream<BlockSize>::CBufferedStream(const CBufferedStream<BlockSize> &_strm) : CBufferedStream() 
    {
        copyStream(_strm);
        reset();
    }

    template<size_t BlockSize>
    inline CBufferedStream<BlockSize>::CBufferedStream(const std::string &_filename) : CBufferedStream() 
    {
        load(_filename);
        reset();
    }

    template<size_t BlockSize>
    inline CBufferedStream<BlockSize> &CBufferedStream<BlockSize>::write(const char *_data, size_t _size)
    {
        size_t totalWritten = 0;
        size_t lastWritten = _size;

        // Allocates enough space to store the data.
        size_t idx = (m_Offset + _size) / BlockSize + 1;
        if(idx >= m_Blocks.size())
            allocateBlocks(idx - m_Blocks.size());

        while (_size > 0)
        {
            idx = m_Offset / BlockSize;            
            SBlock *block = m_Blocks[idx];

            lastWritten = block->write(_data + totalWritten, _size, m_Offset - (idx * BlockSize));
            m_Offset += lastWritten;
            totalWritten += lastWritten;
            _size -= lastWritten;

            m_Size = std::max(m_Offset, m_Size);
        }
        
        return *this;
    }

    template<size_t BlockSize>
    inline size_t CBufferedStream<BlockSize>::read(char *_data, size_t _size)
    {
        size_t readtotal = 0;
        while (_size > 0)
        {
            size_t idx = m_Offset / BlockSize;
            if(idx >= m_Blocks.size())
                break;

            SBlock *block = m_Blocks[idx];
            size_t read = block->read(_data + readtotal, _size, m_Offset - (idx * BlockSize));
            readtotal += read;
            m_Offset += read;
            _size -= read;
        }

        return readtotal;
    }

    template<size_t BlockSize>
    inline std::vector<char> CBufferedStream<BlockSize>::data()
    {
        std::vector<char> ret(m_Size, 0);

        size_t oldOffset = m_Offset;
        m_Offset = 0;
        read(&ret[0], m_Size);
        m_Offset = oldOffset;

        return ret;
    }

    template<size_t BlockSize>
    inline size_t CBufferedStream<BlockSize>::offset()
    {
        return m_Offset;
    }

    template<size_t BlockSize>
    inline size_t CBufferedStream<BlockSize>::size()
    {
        return m_Size;
    }

    template<size_t BlockSize>
    inline void CBufferedStream<BlockSize>::skip(size_t _bytes)
    {
        m_Offset += _bytes;
    }

    template<size_t BlockSize>
    inline void CBufferedStream<BlockSize>::reset()
    {
        m_Offset = 0;
    }

    template<size_t BlockSize>
    inline void CBufferedStream<BlockSize>::load(const std::string &_filename)
    {
        clearData();
        FILE *file = fopen(_filename.c_str(), "rb");
        if(file)
        {
            // Obtain file size;
            fseek(file, 0, SEEK_END);
            m_Size = ftell(file);
            m_Offset = m_Size;
            rewind(file);

            size_t blocks = (m_Size / BlockSize) + (((m_Size % BlockSize) > 0) ? 1 : 0);

            // One block is already created
            allocateBlocks(blocks);

            size_t read = BlockSize;
            size_t idx = 0;
            while (read == BlockSize)
            {
                read = fread(m_Blocks[idx]->Data, 1, BlockSize, file);
                idx++;
            }

            fclose(file);
        }
    }

    template<size_t BlockSize>
    inline void CBufferedStream<BlockSize>::clearData()
    {
        for (auto &&b : m_Blocks)
            delete b;

        m_Blocks.clear();
    }

    template<size_t BlockSize>
    inline void CBufferedStream<BlockSize>::copyStream(const CBufferedStream<BlockSize> &_strm)
    {
        clearData();
        m_Blocks.resize(_strm.m_Blocks.size());
        for (size_t i = 0; i < m_Blocks.size(); i++)
            m_Blocks[i] = new SBlock(*_strm.m_Blocks[i]);
        
        m_Offset = _strm.m_Offset;
        m_Size = _strm.m_Size;
    }

    template<size_t BlockSize>
    inline void CBufferedStream<BlockSize>::allocateBlocks(size_t _blocks)
    {
        for (size_t i = 0; i < _blocks; i++)
            m_Blocks.push_back(new SBlock());
    }

    template<size_t BlockSize>
    inline CBufferedStream<BlockSize> &CBufferedStream<BlockSize>::operator=(const CBufferedStream<BlockSize> &strm)
    {
        copyStream(strm);
        reset();
        return *this;
    }

    template<size_t BlockSize>
    inline CBufferedStream<BlockSize>::~CBufferedStream()
    {
        clearData();
    }

    template<size_t BlockSize>
    inline CBufferedStream<BlockSize>::SBlock::SBlock() : Size(BlockSize) { }

    template<size_t BlockSize>
    inline CBufferedStream<BlockSize>::SBlock::SBlock(const CBufferedStream<BlockSize>::SBlock &_block) : SBlock()
    {
        memcpy(Data, _block.Data, Size);
    }

    template<size_t BlockSize>
    inline size_t CBufferedStream<BlockSize>::SBlock::write(const char *_data, size_t _size, size_t _offset)
    {
        size_t available = (Size - _offset);
        if(_size > available)
            _size = available;

        if(_size)
            memcpy(&Data[_offset], _data, _size);

        return _size;
    }

    template<size_t BlockSize>
    inline size_t CBufferedStream<BlockSize>::SBlock::read(char *_data, size_t _size, size_t _offset)
    {
        size_t readable = (Size - _offset);
        if(_size > readable)
            _size = readable;

        if(_size)
            memcpy(_data, &Data[_offset], _size);

        return _size;
    }
} // namespace VoxelOptimizer

#endif //BINARYSTREAM_HPP