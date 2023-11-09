#ifndef GODOTFILESTREAM_HPP
#define GODOTFILESTREAM_HPP

#include <VCore/VCore.hpp>
#include <File.hpp>
#include <Ref.hpp>

class CGodotFileStream : public VCore::IFileStream
{
    public:
        CGodotFileStream(const std::string &_File, const char *_OpenMode);

        size_t Read(char *_Buffer, size_t _Size) override;
        size_t Write(const char *_Buffer, size_t _Size) override;
        void Seek(size_t _Offset, VCore::SeekOrigin _Origin = VCore::SeekOrigin::CUR) override;
        size_t Tell() override;
        size_t Size() override;
        void Close() override;
    private:
        godot::Ref<godot::File> m_File;
};

#endif