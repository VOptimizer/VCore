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

#include <cstdio>
#include <cstring>
#include <string>
#include <VCore/Math/Vector.hpp>

namespace VCore
{
    /**
     * @brief Defines the origin to seek from.
     */
    enum class SeekOrigin : uint8_t
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
            inline void Write(T p_Data)
            {
                Write((const char*)&p_Data, sizeof(T));
            }

            inline std::string ReadLine()
            {
                std::string result;

                while (!Eof()) 
                {
                    char c = Read<char>();
                    if(c == '\n')
                        break;

                    result += c;
                }

                return result;
            }

            /**
             * @return Returns true if the end of file is reached.
             */
            bool Eof();

            /**
             * @brief Reads data from a file.
             * @param p_Buffer: Buffer to write the read data to.
             * @param p_Size: Size of the buffer.
             * @return Returns the read size.
             */
            virtual uint64_t Read(char *p_Buffer, uint64_t p_Size) = 0;

            /**
             * @brief Writes data to a file.
             * @param p_Buffer: Buffer to write to file.
             * @param p_Size: Size of the buffer.
             * @return Returns the written size.
             */
            virtual uint64_t Write(const char *p_Buffer, uint64_t p_Size) = 0;

            /**
             * @brief Moves the cursor by the given offset from the origin.
             * @param p_Offset: Offset in bytes to move the cursor.
             * @param p_Origin: The seek origin.
             */
            virtual void Seek(uint64_t p_Offset, SeekOrigin p_Origin = SeekOrigin::CUR) = 0;

            /**
             * @return Returns the current cursor position in bytes.
             */
            virtual uint64_t Tell() = 0;

            /**
             * @return Returns the size of the file.
             */
            virtual uint64_t Size() = 0;

            /**
             * @brief Closes the file stream.
             */
            virtual void Close() = 0;

            [[nodiscard]] const std::string &GetFilePath() const { return m_FilePath; }

            virtual ~IFileStream() = default;
        protected:
            std::string m_FilePath;
    };

    template<>
    inline void IFileStream::Write<const char*>(const char* p_Data)
    {
        Write(p_Data, strlen(p_Data));
    }

    template<>
    inline void IFileStream::Write<std::string>(std::string p_Data)
    {
        Write(p_Data.c_str(), p_Data.size());
    }

    class IIOHandler
    {
        public:
          /**
           * @brief Creates a new filestream.
           *
           * @return Returns the newly created filestream.
           */
          virtual IFileStream *Open(const std::string &p_File,
                                    const char *p_OpenMode) = 0;

          /**
           * @brief Deletes a given file.
           */
          virtual void Delete(const std::string &p_File) = 0;

          /**
           * @brief Close and free a given stream.
           */
          virtual void Close(IFileStream *p_Stream) = 0;

          virtual ~IIOHandler() = default;
    };

    class CDefaultFileStream : public IFileStream
    {
        public:
            CDefaultFileStream(const std::string &p_File, const char *p_OpenMode);

            uint64_t Read(char *p_Buffer, uint64_t p_Size) override;
            uint64_t Write(const char *p_Buffer, uint64_t p_Size) override;
            void Seek(uint64_t p_Offset, SeekOrigin p_Origin = SeekOrigin::CUR) override;
            uint64_t Tell() override;
            uint64_t Size() override;
            void Close() override;

            ~CDefaultFileStream() override { Close(); }
        private:
            uint64_t m_Size;
            uint64_t m_InternalPosition;
            FILE *m_File;
    };

    class CDefaultIOHandler : public IIOHandler
    {
        public:
            IFileStream *Open(const std::string &p_File, const char *p_OpenMode) override
            {
                return new CDefaultFileStream(p_File, p_OpenMode);
            }

            void Delete(const std::string &p_File) override
            {
                (void)remove(p_File.c_str());
            }

            void Close(IFileStream *p_Stream) override
            {
                if(p_Stream)
                    delete p_Stream;
            }
    };
}

#endif //BINARYSTREAM_HPP