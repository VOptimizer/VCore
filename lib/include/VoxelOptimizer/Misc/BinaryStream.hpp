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
#include <istream>
#include <iterator>
#include <iostream>
#include <fstream>
#include <ostream>
#include <streambuf>
#include <string>
#include <type_traits>
#include <vector>
#include <VoxelOptimizer/Math/Vector.hpp>

namespace VoxelOptimizer
{
    // template<class _CharT, class _Traits = char_traits<_CharT>> 
    // class CFileBuffer : public basic_streambuf<_CharT, _Traits>
    // {
    //     public:
    //         typedef _CharT                         char_type;
    //         typedef _Traits                        traits_type;
    //         typedef typename traits_type::int_type int_type;
    //         typedef typename traits_type::pos_type pos_type;
    //         typedef typename traits_type::off_type off_type;

    //         CFileBuffer() : m_File(nullptr) {}

    //         /**
    //          * @brief Opens a file.
    //          */
    //         void Open(const std::string &_filename, std::ios_base::openmode _mode = std::ios_base::in | std::ios_base::out);

    //         /**
    //          * @brief Closes a file.
    //          */
    //         void Close();

    //         virtual ~CFileBuffer() 
    //         {
    //             Close();
    //         }

    //     protected:
    //         virtual basic_streambuf* setbuf (char_type* s, std::streamsize n) override;
    //         virtual pos_type seekoff (off_type off, std::ios_base::seekdir way, std::ios_base::openmode which) override;
    //         virtual pos_type seekpos (pos_type pos, std::ios_base::openmode which) override;
    //         virtual int sync() override;
    //         virtual std::streamsize xsgetn (char_type* s, std::streamsize n) override;
    //         virtual int_type underflow() override;
    //         virtual int_type uflow() override;
    //         virtual int_type pbackfail (int_type c) override;
    //         virtual std::streamsize xsputn (const char_type* s, std::streamsize n) override;
    //         virtual int_type overflow (int_type c) override;

    //     private:
    //         FILE *m_File;
    // }

    template<class _CharT, class _Traits = std::char_traits<_CharT>> 
    class CMemoryBuffer : public std::basic_streambuf<_CharT, _Traits>
    {
        public:
            typedef _CharT                         char_type;
            typedef _Traits                        traits_type;
            typedef typename traits_type::int_type int_type;
            typedef typename traits_type::pos_type pos_type;
            typedef typename traits_type::off_type off_type;

            CMemoryBuffer();
            CMemoryBuffer(const char *_data, size_t _size);
            CMemoryBuffer(const CMemoryBuffer<_CharT, _Traits> &_buffer);

            virtual ~CMemoryBuffer();

        protected:
            // Not neccessary
            // virtual basic_streambuf* setbuf (char_type* s, std::streamsize n) override;
            
            virtual pos_type seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which) override;
            virtual pos_type seekpos(pos_type pos, std::ios_base::openmode which) override;

            // Not neccessary
            // virtual int sync() override;

            virtual std::streamsize xsgetn(char_type* s, std::streamsize n) override;
            virtual int_type underflow() override;

            // Not neccessary
            // virtual int_type uflow() override;

            virtual int_type pbackfail(int_type c) override;
            virtual std::streamsize xsputn(const char_type* s, std::streamsize n) override;
            virtual int_type overflow(int_type c) override;

        private:
            const static int BLOCKSIZE = 4096;

            struct SBlock
            {
                public:
                    SBlock();
                    SBlock(const SBlock &_block);

                    size_t write(const char *_data, size_t _size, size_t _offset);
                    size_t read(char *_data, size_t _size, size_t _offset);

                    size_t Size;
                    char Data[BLOCKSIZE];
            };

            void ClearData();
            void AllocateBlocks(size_t _blocks);
            void CopyStream(const CMemoryBuffer<_CharT, _Traits> &_buffer);

            std::vector<SBlock*> m_Blocks;
            size_t m_Size;
            off_type m_Offset;
    };

    class CBinaryStream : public std::iostream
    {
        public:
            CBinaryStream();
            CBinaryStream(const CBinaryStream &_strm);
            CBinaryStream(const char *_data, size_t _size);
            CBinaryStream(std::istream &_strm);
            CBinaryStream(std::ostream &_strm);

            CBinaryStream &operator=(CBinaryStream &&_strm);

            template<class T>
            inline CBinaryStream &operator<<(T val)
            {
                write((char*)&val, sizeof(T));
                return *this;
            }

            template<class T>
            inline CBinaryStream &operator>>(T val)
            {
                read((char*)&val, sizeof(T));
                return *this;
            }

            size_t offset();
            size_t size();
            void reset();
            void skip(size_t _bytes);

            std::vector<char> data() {
                size_t offset = this->offset();
                reset();

                size_t size = this->size();
                std::vector<char> ret(size, 0);

                read(&ret[0], size);

                return ret;
            }

            inline std::basic_streambuf<char> *rdbuf() const
            {
                return std::iostream::rdbuf();
            }

            virtual ~CBinaryStream();

        private:
            std::basic_streambuf<char> *m_StreamBuffer; //!< Just a handy shortcut.
    };

    //////////////////////////////////////////////////
    // CBinaryStream functions
    //////////////////////////////////////////////////

    inline CBinaryStream::CBinaryStream() : std::iostream(new CMemoryBuffer<char>())
    {
        m_StreamBuffer = std::iostream::rdbuf();
    }

    inline CBinaryStream::CBinaryStream(const char *_data, size_t _size) : CBinaryStream()
    {
        write(_data, _size);
        reset();
    }

    inline CBinaryStream::CBinaryStream(std::istream &_strm) : std::iostream(_strm.rdbuf()), m_StreamBuffer(nullptr) { }
    inline CBinaryStream::CBinaryStream(std::ostream &_strm) : std::iostream(_strm.rdbuf()), m_StreamBuffer(nullptr) { }
    inline CBinaryStream::CBinaryStream(const CBinaryStream &_strm)
    {
        this->operator=(std::move(const_cast<CBinaryStream&>(_strm)));
        this->init(m_StreamBuffer);
    }

    inline CBinaryStream::~CBinaryStream()
    {
        if(m_StreamBuffer)
            delete m_StreamBuffer;
    }

    template<>
    inline CBinaryStream &CBinaryStream::operator<<(const Math::Vec3i &val)
    {
        write((char*)val.v, sizeof(val.v));
        return *this;
    }

    template<>
    inline CBinaryStream &CBinaryStream::operator>>(Math::Vec3i &val)
    {
        read((char*)val.v, sizeof(val.v));
        return *this;
    }

    template<>
    inline CBinaryStream &CBinaryStream::operator<<(const Math::Vec3f &val)
    {
        write((char*)val.v, sizeof(val.v));
        return *this;
    }

    template<>
    inline CBinaryStream &CBinaryStream::operator>>(Math::Vec3f &val)
    {
        read((char*)val.v, sizeof(val.v));
        return *this;
    }

    /**
     * @brief Special overload for std::string.
     */
    template<>
    inline CBinaryStream &CBinaryStream::operator<<(const std::string &_val)
    {
        uint32_t len = _val.size();
        write((char*)&len, sizeof(len));
        write(_val.data(), len);

        return *this;
    }

    /**
     * @brief Special overload for std::string.
     */
    template<>
    inline CBinaryStream &CBinaryStream::operator>>(std::string &_val)
    {
        uint32_t len = 0;
        read((char*)&len, sizeof(len));

        _val.resize(len);
        read(&_val[0], len);

        return *this;
    }

    inline size_t CBinaryStream::offset()
    {
        return tellg();
    }

    inline size_t CBinaryStream::size()
    {
        size_t offset = this->offset();
        seekg(0, std::ios::end);
        size_t size = tellg();
        seekg(offset, std::ios::beg);

        return size;
    }

    inline void CBinaryStream::reset()
    {
        seekg(0, std::ios::beg);
    }

    inline void CBinaryStream::skip(size_t _bytes)
    {
        seekg(_bytes, std::ios::cur);
    }

    inline CBinaryStream &CBinaryStream::operator=(CBinaryStream &&_strm)
    {
        if(m_StreamBuffer)
        {
            delete m_StreamBuffer;
            m_StreamBuffer = nullptr;
        }

        std::iostream::operator=(std::move(_strm));
        if(_strm.m_StreamBuffer)
        {
            auto *buf = dynamic_cast<CMemoryBuffer<char>*>(_strm.m_StreamBuffer);
            if(buf)
            {
                m_StreamBuffer = new CMemoryBuffer<char>(*buf);
                std::iostream::rdbuf(m_StreamBuffer);
            }
            else
                std::iostream::rdbuf(_strm.m_StreamBuffer);
        }
        else
            std::iostream::rdbuf(_strm.rdbuf());

        return *this;
    }

    //////////////////////////////////////////////////
    // CMemoryBuffer functions
    //////////////////////////////////////////////////

    template<class _CharT, class _Traits> 
    inline CMemoryBuffer<_CharT, _Traits>::CMemoryBuffer() : m_Size(0), m_Offset(0)
    {
        AllocateBlocks(1);
    }

    template<class _CharT, class _Traits> 
    inline CMemoryBuffer<_CharT, _Traits>::CMemoryBuffer(const char *_data, size_t _size) : CMemoryBuffer<_CharT, _Traits>()
    {
        xsputn(_data, _size);
    }

    template<class _CharT, class _Traits> 
    inline CMemoryBuffer<_CharT, _Traits>::CMemoryBuffer(const CMemoryBuffer<_CharT, _Traits> &_buffer) : CMemoryBuffer()
    {
        CopyStream(_buffer);
    }

    template<class _CharT, class _Traits> 
    inline CMemoryBuffer<_CharT, _Traits>::~CMemoryBuffer()
    {
        ClearData();
    }

    template<class _CharT, class _Traits> 
    inline void CMemoryBuffer<_CharT, _Traits>::ClearData()
    {
        for (auto &&b : m_Blocks)
            delete b;

        m_Blocks.clear();
    }

    template<class _CharT, class _Traits> 
    inline void CMemoryBuffer<_CharT, _Traits>::AllocateBlocks(size_t _blocks)
    {
        for (size_t i = 0; i < _blocks; i++)
            m_Blocks.push_back(new SBlock());
    }

    template<class _CharT, class _Traits> 
    inline void CMemoryBuffer<_CharT, _Traits>::CopyStream(const CMemoryBuffer<_CharT, _Traits> &_buffer)
    {
        ClearData();
        m_Blocks.resize(_buffer.m_Blocks.size());
        for (size_t i = 0; i < m_Blocks.size(); i++)
            m_Blocks[i] = new SBlock(*_buffer.m_Blocks[i]);
        
        m_Offset = _buffer.m_Offset;
        m_Size = _buffer.m_Size;
    }

    template<class _CharT, class _Traits> 
    typename CMemoryBuffer<_CharT, _Traits>::pos_type CMemoryBuffer<_CharT, _Traits>::seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which)
    {
        off_type pos;
        switch (way)
        {
            case std::ios_base::beg:
            {
                pos = off;
            } break;
        
            case std::ios_base::cur:
            {
                pos = m_Offset + off;
            } break;

            case std::ios_base::end:
            {
                pos = m_Size - off;
            } break;

            default:
                return pos_type(off_type(-1));
        }

        if(pos < 0)
            pos = 0;

        // Allocates enough space to store the data.
        size_t idx = pos / BLOCKSIZE + 1;
        if(idx >= m_Blocks.size())
            AllocateBlocks(idx - m_Blocks.size());

        m_Offset = pos;
        return m_Offset;
    }

    template<class _CharT, class _Traits> 
    typename CMemoryBuffer<_CharT, _Traits>::pos_type CMemoryBuffer<_CharT, _Traits>::seekpos(pos_type pos, std::ios_base::openmode which)
    {
        if(pos < 0)
            return pos_type(off_type(-1));

        // Allocates enough space to store the data.
        size_t idx = pos / BLOCKSIZE + 1;
        if(idx >= m_Blocks.size())
            AllocateBlocks(idx - m_Blocks.size());

        m_Offset = pos;
        return m_Offset;
    }

    template<class _CharT, class _Traits> 
    std::streamsize CMemoryBuffer<_CharT, _Traits>::xsgetn(char_type* s, std::streamsize n)
    {
        std::streamsize readtotal = 0;
        while (n > 0)
        {
            size_t idx = m_Offset / BLOCKSIZE;
            if(idx >= m_Blocks.size())
                break;

            SBlock *block = m_Blocks[idx];
            size_t read = block->read(s + readtotal, n, m_Offset - (idx * BLOCKSIZE));
            readtotal += read;
            m_Offset += read;
            n -= read;
        }

        return readtotal;
    }

    template<class _CharT, class _Traits> 
    typename CMemoryBuffer<_CharT, _Traits>::int_type CMemoryBuffer<_CharT, _Traits>::underflow()
    {
        size_t idx = m_Offset / BLOCKSIZE;
        size_t offset = m_Offset - (idx * BLOCKSIZE);
        if(idx >= m_Blocks.size())
            return EOF;

        SBlock *block = m_Blocks[idx];        
        std::basic_streambuf<_CharT, _Traits>::setg(block->Data, &block->Data[offset], &block->Data[block->Size]);

        return *std::basic_streambuf<_CharT, _Traits>::gptr();
    }

    template<class _CharT, class _Traits> 
    typename CMemoryBuffer<_CharT, _Traits>::int_type CMemoryBuffer<_CharT, _Traits>::pbackfail(int_type c)
    {
        m_Offset--;
        if(m_Offset < 0)
        {
            m_Offset = 0;
            return EOF;
        }

        return c;
    }

    template<class _CharT, class _Traits> 
    std::streamsize CMemoryBuffer<_CharT, _Traits>::xsputn(const char_type* s, std::streamsize n)
    {
        size_t totalWritten = 0;
        size_t lastWritten = 0;

        // Allocates enough space to store the data.
        size_t idx = (m_Offset + n) / BLOCKSIZE + 1;
        if(idx >= m_Blocks.size())
            AllocateBlocks(idx - m_Blocks.size());

        while (n > 0)
        {
            idx = m_Offset / BLOCKSIZE;            
            SBlock *block = m_Blocks[idx];

            lastWritten = block->write(s + totalWritten, n, m_Offset - (idx * BLOCKSIZE));
            m_Offset += lastWritten;
            totalWritten += lastWritten;
            n -= lastWritten;

            m_Size = std::max((size_t)m_Offset, m_Size);
        }
        
        return totalWritten;
    }

    template<class _CharT, class _Traits> 
    typename CMemoryBuffer<_CharT, _Traits>::int_type CMemoryBuffer<_CharT, _Traits>::overflow(int_type c)
    {
        size_t idx = m_Offset / BLOCKSIZE + 1;
        if(idx >= m_Blocks.size())
            return EOF;

        char data = c;
        m_Blocks[idx]->write(&data, 1, m_Offset - (idx * BLOCKSIZE));
        return c;
    }

    //////////////////////////////////////////////////
    // CBufferedStream::SBlock functions
    //////////////////////////////////////////////////

    template<class _CharT, class _Traits> 
    inline CMemoryBuffer<_CharT, _Traits>::SBlock::SBlock() : Size(CMemoryBuffer<_CharT, _Traits>::BLOCKSIZE) { }

    template<class _CharT, class _Traits> 
    inline CMemoryBuffer<_CharT, _Traits>::SBlock::SBlock(const CMemoryBuffer<_CharT, _Traits>::SBlock &_block) : SBlock()
    {
        memcpy(Data, _block.Data, Size);
    }

    template<class _CharT, class _Traits> 
    inline size_t CMemoryBuffer<_CharT, _Traits>::SBlock::write(const char *_data, size_t _size, size_t _offset)
    {
        size_t available = (Size - _offset);
        if(_size > available)
            _size = available;

        if(_size)
            memcpy(&Data[_offset], _data, _size);

        return _size;
    }

    template<class _CharT, class _Traits> 
    inline size_t CMemoryBuffer<_CharT, _Traits>::SBlock::read(char *_data, size_t _size, size_t _offset)
    {
        size_t readable = (Size - _offset);
        if(_size > readable)
            _size = readable;

        if(_size)
            memcpy(_data, &Data[_offset], _size);

        return _size;
    }

    //////////////////////////////////////////////////
    // CBufferedStream functions
    //////////////////////////////////////////////////

    // template<size_t BLOCKSIZE>
    // inline CBufferedStream<BLOCKSIZE>::CBufferedStream() : m_Size(0), m_Offset(0)
    // {
    //     allocateBlocks(1);
    // }

    // template<size_t BLOCKSIZE>
    // inline CBufferedStream<BLOCKSIZE>::CBufferedStream(const char *_data, size_t _size) : CBufferedStream() 
    // {
    //     write(_data, _size);
    //     reset();
    // }

    // template<size_t BLOCKSIZE>
    // inline CBufferedStream<BLOCKSIZE>::CBufferedStream(const CBufferedStream<BLOCKSIZE> &_strm) : CBufferedStream() 
    // {
    //     copyStream(_strm);
    //     reset();
    // }

    // template<size_t BLOCKSIZE>
    // inline CBufferedStream<BLOCKSIZE>::CBufferedStream(const std::string &_filename) : CBufferedStream() 
    // {
    //     load(_filename);
    //     reset();
    // }

    // template<size_t BLOCKSIZE>
    // inline CBufferedStream<BLOCKSIZE> &CBufferedStream<BLOCKSIZE>::write(const char *_data, size_t _size)
    // {
    //     size_t totalWritten = 0;
    //     size_t lastWritten = _size;

    //     // Allocates enough space to store the data.
    //     size_t idx = (m_Offset + _size) / BLOCKSIZE + 1;
    //     if(idx >= m_Blocks.size())
    //         allocateBlocks(idx - m_Blocks.size());

    //     while (_size > 0)
    //     {
    //         idx = m_Offset / BLOCKSIZE;            
    //         SBlock *block = m_Blocks[idx];

    //         lastWritten = block->write(_data + totalWritten, _size, m_Offset - (idx * BLOCKSIZE));
    //         m_Offset += lastWritten;
    //         totalWritten += lastWritten;
    //         _size -= lastWritten;

    //         m_Size = std::max(m_Offset, m_Size);
    //     }
        
    //     return *this;
    // }

    // template<size_t BLOCKSIZE>
    // inline size_t CBufferedStream<BLOCKSIZE>::read(char *_data, size_t _size)
    // {
    //     size_t readtotal = 0;
    //     while (_size > 0)
    //     {
    //         size_t idx = m_Offset / BLOCKSIZE;
    //         if(idx >= m_Blocks.size())
    //             break;

    //         SBlock *block = m_Blocks[idx];
    //         size_t read = block->read(_data + readtotal, _size, m_Offset - (idx * BLOCKSIZE));
    //         readtotal += read;
    //         m_Offset += read;
    //         _size -= read;
    //     }

    //     return readtotal;
    // }

    // template<size_t BLOCKSIZE>
    // inline std::vector<char> CBufferedStream<BLOCKSIZE>::data()
    // {
    //     std::vector<char> ret(m_Size, 0);

    //     size_t oldOffset = m_Offset;
    //     m_Offset = 0;
    //     read(&ret[0], m_Size);
    //     m_Offset = oldOffset;

    //     return ret;
    // }

    // template<size_t BLOCKSIZE>
    // inline size_t CBufferedStream<BLOCKSIZE>::offset()
    // {
    //     return m_Offset;
    // }

    // template<size_t BLOCKSIZE>
    // inline size_t CBufferedStream<BLOCKSIZE>::size()
    // {
    //     return m_Size;
    // }

    // template<size_t BLOCKSIZE>
    // inline void CBufferedStream<BLOCKSIZE>::skip(size_t _bytes)
    // {
    //     m_Offset += _bytes;
    // }

    // template<size_t BLOCKSIZE>
    // inline void CBufferedStream<BLOCKSIZE>::reset()
    // {
    //     m_Offset = 0;
    // }

    // template<size_t BLOCKSIZE>
    // inline void CBufferedStream<BLOCKSIZE>::load(const std::string &_filename)
    // {
    //     clearData();
    //     FILE *file = fopen(_filename.c_str(), "rb");
    //     if(file)
    //     {
    //         // Obtain file size;
    //         fseek(file, 0, SEEK_END);
    //         m_Size = ftell(file);
    //         m_Offset = m_Size;
    //         rewind(file);

    //         size_t blocks = (m_Size / BLOCKSIZE) + (((m_Size % BLOCKSIZE) > 0) ? 1 : 0);

    //         // One block is already created
    //         allocateBlocks(blocks);

    //         size_t read = BLOCKSIZE;
    //         size_t idx = 0;
    //         while (read == BLOCKSIZE)
    //         {
    //             read = fread(m_Blocks[idx]->Data, 1, BLOCKSIZE, file);
    //             idx++;
    //         }

    //         fclose(file);
    //     }
    // }

    // template<size_t BLOCKSIZE>
    // inline void CBufferedStream<BLOCKSIZE>::copyStream(const CBufferedStream<BLOCKSIZE> &_strm)
    // {
    //     clearData();
    //     m_Blocks.resize(_strm.m_Blocks.size());
    //     for (size_t i = 0; i < m_Blocks.size(); i++)
    //         m_Blocks[i] = new SBlock(*_strm.m_Blocks[i]);
        
    //     m_Offset = _strm.m_Offset;
    //     m_Size = _strm.m_Size;
    // }
}

#endif //BINARYSTREAM_HPP