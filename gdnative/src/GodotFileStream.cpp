#include <GodotFileStream.hpp>

CGodotFileStream::CGodotFileStream(const std::string &_File, const char *_OpenMode)
{
    m_File.instance();

    int64_t openFlag = 0;
    while (*_OpenMode)
    {
        if(*_OpenMode == 'r')
            openFlag |= godot::File::READ;
        else if(*_OpenMode == 'w')
            openFlag |= godot::File::WRITE;

        _OpenMode++;
    }
    

    m_File->open(_File.c_str(), openFlag);
}

size_t CGodotFileStream::Read(char *_Buffer, size_t _Size)
{
    if(m_File.is_null() || (Tell() + (_Size) > Size()))
        return 0;

    godot::PoolByteArray buffer = m_File->get_buffer(_Size);
    memcpy(_Buffer, buffer.read().ptr(), buffer.size());
    return buffer.size();
}

size_t CGodotFileStream::Write(const char *_Buffer, size_t _Size)
{
    if(m_File.is_null())
        return 0;

    godot::PoolByteArray buffer;
    buffer.resize(_Size);
    memcpy(buffer.write().ptr(), _Buffer, buffer.size());

    m_File->store_buffer(buffer);
    return buffer.size();
}

void CGodotFileStream::Seek(size_t _Offset, VCore::SeekOrigin _Origin)
{
    if(m_File.is_null() || _Offset > Size())
        return;

    switch (_Origin)
    {
        case VCore::SeekOrigin::BEG: m_File->seek(_Offset); break;
        case VCore::SeekOrigin::CUR: m_File->seek(Tell() + _Offset); break;
        case VCore::SeekOrigin::END: m_File->seek_end(_Offset); break;
    }
}

size_t CGodotFileStream::Tell()
{
    if(m_File.is_null())
        return 0;

    return m_File->get_position();
}

size_t CGodotFileStream::Size()
{
    if(m_File.is_null())
        return 0;

    return m_File->get_len();
}

void CGodotFileStream::Close()
{
    if(m_File.is_null())
        return;

    m_File->close();
}