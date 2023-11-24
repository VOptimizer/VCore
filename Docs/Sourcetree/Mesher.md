# Mesher

This document tries to describe how the meshing process works, and how to implement a new mesher.

[IMesher.hpp](../../lib/include/VCore/Meshing/IMesher.hpp) represents the public interface of a mesher. It handles creation of a mesher instance via the `IMesher::Create` function. Each library mesher must be created inside this function.

Each public `Generate*` method of the mesher calls internally `GenerateChunks` which splits the `VoxelModel` into multiple chunks and assigns them to different thread.

Each thread which `GenerateChunks` creates calls the the protected overritten `GenerateMeshChunk` method. This is the only method a new mesher needs to be override. Please keep in mind, that this method is called by multiple threads, so every member variable needs to be locked using a mutex or semaphore.

All currently available mesher implementations can be found [here](../../lib/src/Meshing/Implementations/).

## Basic example

```c++
#ifndef MYMESHER_HPP
#define MYMESHER_HPP

#include <vector>
#include <VCore/Meshing/IMesher.hpp>

namespace VCore
{
    class CMyMesher : public IMesher
    {
        public:
            CMyMesher() : IMesher() {}
            virtual ~CMyMesher() = default;
        protected:
            // Please don't put the implementation inside the header file.
            SMeshChunk GenerateMeshChunk(VoxelModel m, const SChunkMeta &_Chunk, bool) override
            {
                // Mesh the given chunk.

                SMeshChunk chunk;
                chunk.UniqueId = _Chunk.UniqueId;
                chunk.InnerBBox = _Chunk.InnerBBox;
                chunk.TotalBBox = _Chunk.TotalBBox;
                chunk.MeshData = mesh;

                return chunk;
            }
    };
}

#endif
```