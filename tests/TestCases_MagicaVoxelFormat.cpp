#include "StringMakerSpecializations.hpp"

#include <VCore/VCore.hpp>

#include "../lib/src/Formats/Implementations/MagicaVoxel/MagicaVoxelFormat.hpp"

#include "VoxelTestFormat.hpp"

TEST_CASE("load MagicaVoxel .vox file -> single model -> no streaming")
{
    const std::string FILE = "../models/single/Axis.vox";
    VCore::CMagicaVoxelFormat voxLoader;
    voxLoader.Open(FILE, VCore::FileMode::READ);

    voxLoader.Load();
    REQUIRE(voxLoader.SceneTree != nullptr);
    REQUIRE(voxLoader.SceneTree->GetModels().size() > 0);
    REQUIRE(voxLoader.SceneTree->GetModels().size() == 1);

    CVoxelTestFormat test(FILE);
    test.ValidateModel(voxLoader.SceneTree->GetModels()[0]);
}