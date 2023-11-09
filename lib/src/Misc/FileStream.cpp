#include <VCore/Misc/FileStream.hpp>

namespace VCore
{
    bool IFileStream::Eof()
    {
        return Tell() >= Size();
    }

    CDefaultFileStream::CDefaultFileStream(const std::string &_File, const char *_OpenMode)
    {
        m_File = fopen(_File.c_str(), _OpenMode);
        if(m_File)
        {
            Seek(0, SeekOrigin::END);
            m_Size = Tell();
            Seek(0, SeekOrigin::BEG);
        }
    }

    size_t CDefaultFileStream::Read(char *_Buffer, size_t _Size)
    {
        return fread(_Buffer, 1, _Size, m_File);
    }

    size_t CDefaultFileStream::Write(const char *_Buffer, size_t _Size)
    {
        return fwrite(_Buffer, 1, _Size, m_File);
    }

    void CDefaultFileStream::Seek(size_t _Offset, SeekOrigin _Origin)
    {
        int seekOff = 0;
        switch (_Origin)
        {
            case SeekOrigin::BEG: seekOff = SEEK_SET; break;
            case SeekOrigin::CUR: seekOff = SEEK_CUR; break;
            case SeekOrigin::END: seekOff = SEEK_END; break;
        }

        fseek(m_File, _Offset, seekOff);
    }

    size_t CDefaultFileStream::Tell()
    {
        return ftell(m_File);
    }

    size_t CDefaultFileStream::Size()
    {
        return m_Size;
    }

    void CDefaultFileStream::Close()
    {
        if(m_File)
        {
            fclose(m_File);
            m_File = nullptr;
        }
    }
} // namespace VCore
