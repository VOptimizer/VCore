#include "VCore/Math/Vector.hpp"
#include "VCore/VConfig.hpp"
#include "VCore/Voxel/Storage/Chunk.hpp"
#include "VCore/Voxel/Voxel.hpp"
#include <VCore/VCore.hpp>
#include <cstdint>

#include "StringMakerSpecializations.hpp"

TEST_CASE("insert voxel into chunk")
{
    VCore::CChunk chunk(nullptr);

    const auto position = VCore::Math::Vec3i(1, 1, 1);
    const auto voxel = VCore::CVoxel(5, 2);

    CHECK(chunk.insert({position, voxel}) == true);
    CHECK(chunk.Mask.GetRowFaces(position, 0) == 0x2);

    CHECK(chunk.IsDirty == true);
    CHECK(chunk.IsEmpty() == false);

    auto bbox = chunk.inner_bbox(VCore::Math::Vec3i::ZERO);
    CHECK_EQ(bbox.GetSize(), VCore::Math::Vec3i::ONE);
    CHECK_EQ(bbox.Beg, position);
    CHECK_EQ(bbox.End, position);

    auto storedVoxel = chunk.find(position);
    CHECK_EQ(storedVoxel, voxel);
}

TEST_CASE("insert voxel into chunk -> erase voxel -> empty")
{
    VCore::CChunk chunk(nullptr);

    const auto position = VCore::Math::Vec3i(1, 1, 1);
    const auto voxel = VCore::CVoxel(5, 2);

    chunk.insert({position, voxel});

    auto bbox = chunk.inner_bbox(VCore::Math::Vec3i::ZERO);
    VCore::CVoxelSpaceIterator it(nullptr, bbox, {position, voxel});
    chunk.erase(it);
    
    CHECK(chunk.Mask.GetRowFaces(position, 0) == 0);
    CHECK(chunk.IsEmpty() == true);
}

TEST_CASE("insert voxels until upgrade needed")
{
    VCore::CChunk chunk(nullptr);

    auto startPosition = VCore::Math::Vec3i(1, 1, 1);
    for (uint32_t i = 0; i < 254; i++) 
    {
        auto result = chunk.insert({startPosition, VCore::CVoxel(i, 0)});
        CHECK(result == true);

        startPosition.x++;
        if (startPosition.x >= VCore::Config::ChunkSize - 1) 
        {
            startPosition.x = 1;
            startPosition.y++;
            if(startPosition.y >= VCore::Config::ChunkSize - 1)
            {
                startPosition.y = 1;
                startPosition.z++;
            }
        }
    }

    auto result = chunk.insert({startPosition, VCore::CVoxel(5475847, 0)});
    CHECK(result == false);
}

TEST_CASE("get voxel by position")
{
    VCore::CChunk chunk(nullptr);

    const auto position1 = VCore::Math::Vec3i(1, 1, 1);
    const auto voxel1 = VCore::CVoxel(5, 2);

    const auto position2 = VCore::Math::Vec3i(10, 10, 10);
    const auto voxel2 = VCore::CVoxel(32, 3);

    chunk.insert({position1, voxel1});
    chunk.insert({position2, voxel2});

    auto vox1Find = chunk.find(position1);
    CHECK_EQ(vox1Find, voxel1);

    auto vox2Find = chunk.find(position2);
    CHECK_EQ(vox2Find, voxel2);
}

TEST_CASE("get next valid voxel from position")
{
    VCore::CChunk chunk(nullptr);

    const auto position1 = VCore::Math::Vec3i(1, 1, 1);
    const auto voxel1 = VCore::CVoxel(5, 2);

    const auto position2 = VCore::Math::Vec3i(10, 10, 10);
    const auto voxel2 = VCore::CVoxel(32, 3);

    chunk.insert({position1, voxel1});
    chunk.insert({position2, voxel2});

    auto nextVox = chunk.next(position1 + VCore::Math::Vec3i::ONE);
    CHECK_EQ(nextVox.first, position2);
    CHECK_EQ(nextVox.second, voxel2);

    nextVox = chunk.next(position2 + VCore::Math::Vec3i::ONE);
    CHECK_EQ(nextVox.first, VCore::Math::Vec3i());
    CHECK_EQ(nextVox.second, VCore::CVoxel());
}

TEST_CASE("insert voxel into chunk -> erase voxel -> get next")
{
    VCore::CChunk chunk(nullptr);

    const auto position1 = VCore::Math::Vec3i(1, 1, 1);
    const auto voxel1 = VCore::CVoxel(5, 2);

    const auto position2 = VCore::Math::Vec3i(10, 10, 10);
    const auto voxel2 = VCore::CVoxel(32, 3);

    chunk.insert({position1, voxel1});
    chunk.insert({position2, voxel2});

    auto bbox = chunk.inner_bbox(VCore::Math::Vec3i::ZERO);
    VCore::CVoxelSpaceIterator it(nullptr, bbox, {position1, voxel1});
    auto nextVox = chunk.erase(it);

    CHECK_EQ(nextVox.first, position2);
    CHECK_EQ(nextVox.second, voxel2);
}

TEST_CASE("upgrade chunk to full storage")
{
    VCore::CChunk chunk(nullptr);

    const auto position1 = VCore::Math::Vec3i(1, 1, 1);
    const auto voxel1 = VCore::CVoxel(5, 2);

    const auto position2 = VCore::Math::Vec3i(10, 10, 10);
    const auto voxel2 = VCore::CVoxel(32, 3);

    chunk.insert({position1, voxel1});
    chunk.insert({position2, voxel2});

    // Allocates a new storage
    chunk.Upgrade();

    // Expect all voxels are copied over to the new storage.
    auto vox1Find = chunk.find(position1);
    CHECK_EQ(vox1Find, voxel1);

    auto vox2Find = chunk.find(position2);
    CHECK_EQ(vox2Find, voxel2);
}