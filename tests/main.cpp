#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "StringMakerSpecializations.hpp"

#include <VCore/VCore.hpp>

#include "../lib/src/Formats/Implementations/MagicaVoxel/MagicaVoxelFormat.hpp"

TEST_CASE("load MagicaVoxel .vox file -> single model -> no streaming")
{
    VCore::CMagicaVoxelFormat voxLoader;
    voxLoader.Open("../models/single/Axis.vox", VCore::FileMode::READ);

    voxLoader.Load();
    REQUIRE(voxLoader.SceneTree != nullptr);
    REQUIRE(voxLoader.SceneTree->GetModels().size() > 0);
    CHECK(voxLoader.SceneTree->GetModels().size() == 1);

    
}

// int factorial(int number) { return number <= 1 ? number : factorial(number - 1) * number; }

// TEST_CASE("testing the factorial function") 
// {
//     CHECK(factorial(1) == 2);
//     CHECK(factorial(2) == 2);
//     CHECK(factorial(3) == 6);
//     CHECK(factorial(10) == 3628800);
// }