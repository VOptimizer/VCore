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

You can define your data types for the `Mesh` class, making it easier to convert to a mesh instance in your engine or framework. Currently, this change of data types is only possible via the [VConfig.hpp](../../lib/include/VCore/VConfig.hpp) file at compile time.

For proper functionality, it is important that all methods do not change their signature, with the exception of the information if it is a reference.

There are two ways to overwrite this file: either by replacing the original one with yours or by adding it to the include list before all other files. Refer to [gdnative](../../gdnative/CMakeLists.txt) for an example