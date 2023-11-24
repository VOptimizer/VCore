# Voxelformat

This document tries to describe how the import process works, and how to implement a new importer.

[IVoxelFormat.hpp](../../lib/include/VCore/Formats/IVoxelFormat.hpp) represents the public interface of an importer. It handles creation of an importer instance via the `IVoxelFormat::Create` or `IVoxelFormat::CreateAndLoad` function. Each library importer must be created inside this function.

Internally the `Load` funtion calls the protected method `ParseFormat`, which then parses the loaded voxel file. All loaded models, animations, material and textures must be added to their corresponding collection. Also each format must create a `SceneTree`. Also it's important, that a model is either inside an animation or inside the `m_Models` array, not both!

All currently available importer implementations can be found [here](../../lib/src/Formats/Implementations/).

## Basic example

```c++
#ifndef MYIMPORTER_HPP
#define MYIMPORTER_HPP

#include <VCore/Formats/IVoxelFormat.hpp>
#include "Kenshape.hpp"

namespace VCore
{
    class CMyImporter : public IVoxelFormat
    {
        public:
            CMyImporter() = default;
            ~CMyImporter() = default;
        private:
            // Please don't put the implementation inside the header file.
            void ParseFormat() override
            {
                // Load each material
                // Add each material to m_Materials

                // Load each model
                // Add each model which is not part of an animation to m_Models

                // Load each texture / color
                // Add each texture to m_Textures

                // Load each animation
                // Add each animation to m_Animations
            }
    };
}


#endif
```