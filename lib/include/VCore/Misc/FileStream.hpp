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
#include <string>
#include <vector>
#include <VCore/Math/Vector.hpp>

namespace VCore
{
    /**
     * @brief Defines the origin to seek from.
     */
    enum class SeekOrigin
    {
        BEG,
        CUR,
        END
    };

    /**
     * @brief Interface for any kind of file operation.
     */
    class IFileStream
    {
        public:
            IFileStream() = default;
            IFileStream(const IFileStream&) = delete;
            IFileStream(IFileStream&&) = delete;

            IFileStream& operator=(const IFileStream&) = delete;
            IFileStream& operator=(IFileStream&&) = delete;

            /**
             * @return Reads data from a file and returns the data as type T.
             */
            template<class T>
            T Read()
            {
                T retVal;
                Read((char*)&retVal, sizeof(T));
                return retVal;
            }

            /**
             * @return Returns true if the end of file is reached.
             */
            bool Eof();

            /**
             * @brief Reads data from a file.
             * @param _Buffer: Buffer to write the read data to.
             * @param _Size: Size of the buffer.
             * @return Returns the read size.
             */
            virtual size_t Read(char *_Buffer, size_t _Size) = 0;

            /**
             * @brief Writes data to a file.
             * @param _Buffer: Buffer to write to file.
             * @param _Size: Size of the buffer.
             * @return Returns the written size.
             */
            virtual size_t Write(const char *_Buffer, size_t _Size) = 0;

            /**
             * @brief Moves the cursor by the given offset from the origin.
             * @param _Offset: Offset in bytes to move the cursor.
             * @param _Origin: The seek origin.
             */
            virtual void Seek(size_t _Offset, SeekOrigin _Origin = SeekOrigin::CUR) = 0;

            /**
             * @return Returns the current cursor position in bytes.
             */
            virtual size_t Tell() = 0;

            /**
             * @return Returns the size of the file.
             */
            virtual size_t Size() = 0;

            /**
             * @brief Closes the file stream.
             */
            virtual void Close() {}

            virtual ~IFileStream() { Close(); }
    };

    class CDefaultFileStream : public IFileStream
    {
        public:
            CDefaultFileStream(const std::string &_File, const char *_OpenMode);

            size_t Read(char *_Buffer, size_t _Size) override;
            size_t Write(const char *_Buffer, size_t _Size) override;
            void Seek(size_t _Offset, SeekOrigin _Origin = SeekOrigin::CUR) override;
            size_t Tell() override;
            size_t Size() override;
            void Close() override;

            virtual ~CDefaultFileStream() = default;
        private:
            size_t m_Size;
            FILE *m_File;
    };

    // class CBinaryStream
    // {
    //     public:
    //         CBinaryStream() {}
    //         CBinaryStream(const CBinaryStream &_Other) = delete;
    //         CBinaryStream(CBinaryStream &&_Other) { *this = std::move(_Other); }

    //         CBinaryStream &operator=(CBinaryStream &&_Other);
    //         CBinaryStream &operator=(const CBinaryStream &_Other) = delete;

    //         template<class T>
    //         inline CBinaryStream &operator<<(T val)
    //         {
    //             write((char*)&val, sizeof(T));
    //             return *this;
    //         }

    //         template<class T>
    //         inline CBinaryStream &operator>>(T val)
    //         {
    //             read((char*)&val, sizeof(T));
    //             return *this;
    //         }

    //         size_t offset();
    //         size_t size();
    //         void reset();
    //         void skip(size_t _bytes);

    //         std::vector<char> data() {
    //             reset();

    //             size_t size = this->size();
    //             std::vector<char> ret(size, 0);

    //             read(&ret[0], size);

    //             return ret;
    //         }

    //         size_t read(char *_Buffer, size_t _Size);
    //         size_t write(const char *_Buffer, size_t _Size);

    //         virtual ~CBinaryStream() = default;

    //     private:
    //         CDefaultFileStream m_StreamBuffer;
    // };

    // //////////////////////////////////////////////////
    // // CBinaryStream functions
    // //////////////////////////////////////////////////

    // template<>
    // inline CBinaryStream &CBinaryStream::operator<<(const Math::Vec3i &val)
    // {
    //     write((char*)val.v, sizeof(val.v));
    //     return *this;
    // }

    // template<>
    // inline CBinaryStream &CBinaryStream::operator>>(Math::Vec3i &val)
    // {
    //     read((char*)val.v, sizeof(val.v));
    //     return *this;
    // }

    // template<>
    // inline CBinaryStream &CBinaryStream::operator<<(const Math::Vec3f &val)
    // {
    //     write((char*)val.v, sizeof(val.v));
    //     return *this;
    // }

    // template<>
    // inline CBinaryStream &CBinaryStream::operator>>(Math::Vec3f &val)
    // {
    //     read((char*)val.v, sizeof(val.v));
    //     return *this;
    // }

    // /**
    //  * @brief Special overload for std::string.
    //  */
    // template<>
    // inline CBinaryStream &CBinaryStream::operator<<(const std::string &_val)
    // {
    //     uint32_t len = _val.size();
    //     write((char*)&len, sizeof(len));
    //     write(_val.data(), len);

    //     return *this;
    // }

    // /**
    //  * @brief Special overload for std::string.
    //  */
    // template<>
    // inline CBinaryStream &CBinaryStream::operator>>(std::string &_val)
    // {
    //     uint32_t len = 0;
    //     read((char*)&len, sizeof(len));

    //     _val.resize(len);
    //     read(&_val[0], len);

    //     return *this;
    // }

    // inline size_t CBinaryStream::offset()
    // {
    //     return m_StreamBuffer.offset();
    // }

    // inline size_t CBinaryStream::size()
    // {
    //     return m_StreamBuffer.size();
    // }

    // inline void CBinaryStream::reset()
    // {
    //     m_StreamBuffer.reset();
    // }

    // inline void CBinaryStream::skip(size_t _bytes)
    // {
    //     m_StreamBuffer.skip(_bytes);
    // }

    // inline CBinaryStream &CBinaryStream::operator=(CBinaryStream &&_Other)
    // {
    //     m_StreamBuffer = std::move(_Other.m_StreamBuffer);
    //     return *this;
    // }

    // inline size_t CBinaryStream::read(char *_Buffer, size_t _Size)
    // {
    //     return m_StreamBuffer.read(_Buffer, _Size);
    // }

    // inline size_t CBinaryStream::write(const char *_Buffer, size_t _Size)
    // {
    //     return m_StreamBuffer.write(_Buffer, _Size);
    // }
}

#endif //BINARYSTREAM_HPP