# VCore customisations

VCore allows you to customize the library so that you can integrate it seamlessly into your engine or framework.

## File handling

VCore provides two essential interfaces that must be implemented to use your file API with the library. Both interfaces, along with the default file stream implementation, can be found in the [FileStream.hpp](../../lib/include/VCore/Misc/FileStream.hpp) file.

### `IFileStream` Interface

`IFileStream` is the primary interface for reading and writing files. The following methods must be implemented:
```c++
class CMyFileStream : public IFileStream
{
    public:
        // Example of opening a file; read on to find out more about how to open files.
        CMyFileStream(const std::string &_File, const char *_OpenMode)
        {
            // Open _File with the _OpenMode flags.
        }

        size_t Read(char *_Buffer, size_t _Size) override
        {
            // Read data and return the read size.
        }

        size_t Write(const char *_Buffer, size_t _Size) override
        {
            // Write data and return the written size.
        }

        void Seek(size_t _Offset, SeekOrigin _Origin = SeekOrigin::CUR) override
        {
            // Seek _Offset (Bytes) from origin _Origin.
        }

        size_t Tell() override
        {
            // Return the current cursor position (Bytes)
        }

        size_t Size() override
        {
            // Return the file size in bytes.
        }

        void Close() override
        {
            // Close the file
        }

        virtual ~CMyFileStream() = default;
};
```

### `IIOHandler` Interface

`IIOHandler` provides an interface to create a new instance of your file stream. This is the main interface expected by exporters and importers to create a new file stream.
```c++
class CMyIOHandler : public IIOHandler
{
    public:
        IFileStream *Open(const std::string &_File, const char *_OpenMode)
        {
            // Create a new instance of your filestream
            return new CMyFileStream(_File, _OpenMode);
        }

        void Close(IFileStream *_Stream)
        {
            // Release your filestream.
            if(_Stream)
                delete _Stream;
        }
};
```

## Customize output Mesh

For less type conversions during runtime, V-Core offers an API to directly use the data structures of your engine / framework. There are two ways to let V-Core use your data structures, either by inheriting `ISurface` or even easier by using the template class `TSurface` (your arrays must be STL compliant).

Example
```c++
using MyEngineSurface = TSurface<std::vector<SVertex>, std::vector<uint32_t>, UINT32_MAX>;

// Usage

auto Mesher = VCore::IMesher::Create<MyEngineSurface>(VCore::MesherTypes::SIMPLE);
```