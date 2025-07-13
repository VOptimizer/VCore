#include "VCore/Math/Vector.hpp"
#include "VCore/VConfig.hpp"
#include "VCore/Voxel/Storage/Chunk.hpp"
#include "VCore/Voxel/Storage/VoxelSpace.hpp"
#include "VCore/Voxel/Voxel.hpp"
#include "StringMakerSpecializations.hpp"

#include <VCore/VCore.hpp>
#include <cstddef>
#include <cstdint>

void CheckChunks(const VCore::CVoxelSpace::querylist &p_Chunks, uint32_t p_ChunkCount)
{
    uint32_t counter = 0;
    for (auto &&chunk : p_Chunks) 
        counter++;

    CHECK(counter == p_ChunkCount);
}

TEST_CASE("insert single voxel -> find voxel -> voxel equals inserted")
{
    VCore::CVoxelSpace space;

    const auto position = VCore::Math::Vec3i(0, 1, 0);
    const auto voxel = VCore::CVoxel(5, 2);

    space.Insert({
        position,
        voxel
    });

    CHECK(space.Size() == 1);
    CheckChunks(space.QueryChunks(), 1);
    auto it = space.Find(position);
    REQUIRE_NE(it, space.end());

    CHECK(it->second == voxel);
}

TEST_CASE("insert single voxel -> erase single voxel")
{
    VCore::CVoxelSpace space;

    const auto position = VCore::Math::Vec3i(0, 1, 0);
    const auto voxel = VCore::CVoxel(5, 2);

    space.Insert({
        position,
        voxel
    });

    CHECK(space.Size() == 1);

    auto it = space.Find(position);
    space.Erase(it);

    CHECK(space.Size() == 0);
    CheckChunks(space.QueryChunks(), 0);
}

TEST_CASE("insert multiple voxels -> multiple chunks")
{
    VCore::CVoxelSpace space;

    const auto position = VCore::Math::Vec3i(0, 0, 0);
    const auto position1 = VCore::Math::Vec3i(VCore::Config::ChunkSize, 0, 0);
    const auto voxel = VCore::CVoxel(5, 2);

    space.Insert({position, voxel});
    space.Insert({position1, voxel});

    CheckChunks(space.QueryChunks(), 2);
    CheckChunks(space.QueryDirtyChunks(), 2);
}
