#include <VCore/VConfig.hpp>
#include <VCore/Misc/FileStream.hpp>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace VCore
{
    bool IFileStream::Eof()
    {
        return Tell() >= Size();
    }

    uint64_t IFileStream::Read(char *p_Buffer, uint64_t p_Size)
    {
        uint64_t read = 0;
        while (read < p_Size) 
        {
            if(m_ReadPos >= m_Buffer.size())
            {
                FillBuffer();
                if(m_Buffer.size() == 0)
                    break;
            }

            const uint64_t remainingBufferSize = m_Buffer.size() - m_ReadPos;
            const uint64_t remainingReadSize = p_Size - read;
            const uint64_t readSize = (remainingReadSize > remainingBufferSize) ? remainingBufferSize : remainingReadSize;

            memcpy(p_Buffer + read, m_Buffer.data() + m_ReadPos, readSize);
            read += readSize;
            m_ReadPos += readSize;
        }
        return read;
    }

    void IFileStream::Seek(int64_t p_Offset, SeekOrigin p_Origin)
    {
        switch (p_Origin)
        {
            case SeekOrigin::BEG: 
            {
                if(TellInternal() - m_Buffer.size() == 0 && p_Offset > 0 && p_Offset < m_Buffer.size())
                {
                    m_ReadPos = p_Offset;
                    return;
                }
            } break;

            case SeekOrigin::CUR: 
            {
                if(m_ReadPos + p_Offset < m_Buffer.size() && m_ReadPos + p_Offset > 0)
                {
                    m_ReadPos += p_Offset;
                    return;
                }
                
                p_Offset = p_Offset - m_ReadPos;
            } break;

            case SeekOrigin::END: 
            {
                if(m_ReadPos + p_Offset < m_Buffer.size() && m_ReadPos + p_Offset > 0)
                {
                    m_ReadPos = m_Buffer.size() + p_Offset;
                    return;
                }
            } break;
        }

        SeekInternal(p_Offset, p_Origin);
        FillBuffer();
    }

    uint64_t IFileStream::Tell()
    {
        return TellInternal() - m_Buffer.size() + m_ReadPos;
    }

    void IFileStream::FillBuffer()
    {
        const uint64_t remainingSize = Size() - TellInternal();
        const uint64_t readSize = (Config::MaxFileReadBufferSize > remainingSize) ? remainingSize : Config::MaxFileReadBufferSize;
        m_Buffer.resize(readSize);
        ReadInternal((char*)m_Buffer.data(), readSize);
        m_ReadPos = 0;
    }

    CDefaultFileStream::CDefaultFileStream(const std::string &p_File, const char *p_OpenMode)
    {
        m_File = fopen(p_File.c_str(), p_OpenMode);
        if(m_File)
        {
            Seek(0, SeekOrigin::END);
            m_Size = Tell();
            Seek(0, SeekOrigin::BEG);
            m_FilePath = p_File;
            m_InternalPosition = 0;
        }
    }

    uint64_t CDefaultFileStream::ReadInternal(char *p_Buffer, uint64_t p_Size)
    {
        auto read = fread(p_Buffer, 1, p_Size, m_File);
        m_InternalPosition += read;
        return read;
    }

    uint64_t CDefaultFileStream::Write(const char *p_Buffer, uint64_t p_Size)
    {
        auto written = fwrite(p_Buffer, 1, p_Size, m_File);
        m_InternalPosition += written;

        if(m_InternalPosition > m_Size)
            m_Size += written;
        else
        fflush(m_File);

        return  written;
    }

    void CDefaultFileStream::SeekInternal(int64_t p_Offset, SeekOrigin p_Origin)
    {
        int seekOff = 0;
        switch (p_Origin)
        {
            case SeekOrigin::BEG: seekOff = SEEK_SET; break;
            case SeekOrigin::CUR: seekOff = SEEK_CUR; break;
            case SeekOrigin::END: seekOff = SEEK_END; break;
        }

        fseek(m_File, p_Offset, seekOff);
        m_InternalPosition = Tell();
    }

    uint64_t CDefaultFileStream::TellInternal()
    {
        return ftell(m_File);
    }

    uint64_t CDefaultFileStream::Size()
    {
        return m_Size;
    }

    void CDefaultFileStream::Close()
    {
        if(m_File)
        {
            fclose(m_File);
            m_File = nullptr;
            m_Buffer.clear();
            m_ReadPos = 0;
            m_InternalPosition = 0;
        }
    }
} // namespace VCore
