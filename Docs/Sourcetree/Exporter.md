# Exporter

This document tries to describe how the export process works, and how to implement a new exporter.

[IExporter.hpp](../../lib/include/VCore/Export/IExporter.hpp) represents the public interface of an exporter. It handles creation of an exporter instance via the `IExporter::Create` function. Each library exporter must be created inside this function.

Internally the `Save` method calls the `Generate(std::vector<Mesh>)` method.

All currently available exporter implementations can be found [here](../../lib/src/Export/Implementations/).

## Basic example

```c++
#ifndef MYEXPORTER_HPP
#define MYEXPORTER_HPP

#include <VCore/Export/IExporter.hpp>

namespace VCore
{
    class CMyExporter : public IExporter
    {
        public:
            CMyExporter() = default;

            // Please don't put the implementation inside the header file.
            std::map<std::string, std::vector<char>> Generate(std::vector<Mesh> Meshes) override
            {
                // Iterate over each mesh an write the data to the data stream

                // Return each file as map e.g.: {{"mtl", MTLSTREAM}, {"obj", OBJSTREAM}}
            }

            ~CMyExporter() = default;
    };
}


#endif
```