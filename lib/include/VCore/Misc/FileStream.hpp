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
#include <string.h>
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

            template<class T>
            inline void Write(T _Data)
            {
                Write((const char*)&_Data, sizeof(T));
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

    template<>
    inline void IFileStream::Write<const char*>(const char* _Data)
    {
        Write(_Data, strlen(_Data));
    }

    template<>
    inline void IFileStream::Write<std::string>(std::string _Data)
    {
        Write(_Data.c_str(), _Data.size());
    }

    class IIOHandler
    {
        public:
            /**
             * @brief Creates a new filestream.
             * 
             * @return Returns the newly created filestream.
             */
            virtual IFileStream *Open(const std::string &_File, const char *_OpenMode) = 0;

            /**
             * @brief Close and free a given stream.
             */
            virtual void Close(IFileStream *_Stream) = 0;

            virtual ~IIOHandler() = default;
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

    class CDefaultIOHandler : public IIOHandler
    {
        public:
            IFileStream *Open(const std::string &_File, const char *_OpenMode)
            {
                return new CDefaultFileStream(_File, _OpenMode);
            }

            void Close(IFileStream *_Stream)
            {
                if(_Stream)
                    delete _Stream;
            }
    };
}

#endif //BINARYSTREAM_HPP