#include "StringMakerSpecializations.hpp"
#include <VCore/VCore.hpp>
#include <cstdint>
#include <memory>

TEST_CASE("Frustum culling -> simulated -> on frustum")
{
    VCore::CFrustum frustum = VCore::CFrustum::Create(
        VCore::Math::Vec3f(10, 1, 10), 
        VCore::Math::Vec3f::FRONT,
        VCore::Math::Vec3f::UP.cross(VCore::Math::Vec3f::FRONT),
        VCore::Math::Vec3f::RIGHT.cross(VCore::Math::Vec3f::FRONT),
        1920.f / 1080.f, 
        90.f * (M_PI / 180.f), 
        0.05f, 
        1000.f
    );

    VCore::CBBox bbox(VCore::Math::Vec3i(10, 0, 20), VCore::Math::Vec3i(40, 20, 40));
    CHECK_EQ(frustum.IsOnFrustum(bbox), true);
}

TEST_CASE("Frustum culling -> simulated -> not on frustum")
{
    VCore::CFrustum frustum = VCore::CFrustum::Create(
        VCore::Math::Vec3f(10, 1, 10), 
        VCore::Math::Vec3f::FRONT,
        VCore::Math::Vec3f::UP.cross(VCore::Math::Vec3f::FRONT),
        VCore::Math::Vec3f::RIGHT.cross(VCore::Math::Vec3f::FRONT),
        1920.f / 1080.f, 
        90.f * (M_PI / 180.f), 
        0.05f, 
        1000.f
    );

    VCore::CBBox bbox(VCore::Math::Vec3i(10, 0, -40), VCore::Math::Vec3i(40, 20, 0));
    CHECK_EQ(frustum.IsOnFrustum(bbox), false);
}

TEST_CASE("Frustum culling -> multiple models in scene tree")
{
    VCore::CFrustum frustum = VCore::CFrustum::Create(
        VCore::Math::Vec3f(3 * 128, 137, 3 * 128), 
        VCore::Math::Vec3f::FRONT,
        VCore::Math::Vec3f::UP.cross(VCore::Math::Vec3f::FRONT),
        VCore::Math::Vec3f::RIGHT.cross(VCore::Math::Vec3f::FRONT),
        1920.f / 1080.f, 
        90.f * (M_PI / 180.f), 
        0.05f, 
        1000.f
    );

    auto loader = VCore::IVoxelFormat::CreateAndOpen("../models/frustum.vox", VCore::FileMode::READ);
    loader->Load();

    loader->SceneTree->UpdateBoundingVolumes(loader->SceneTree->GetModels());
    auto nodes = loader->SceneTree->DoFrustumCulling(frustum);

    CHECK_LT(nodes.size(), loader->SceneTree->GetModels().size());
    CHECK_GT(nodes.size(), 0);
}

TEST_CASE("Frustum culling -> cull chunks")
{
    VCore::CFrustum frustum = VCore::CFrustum::Create(
        VCore::Math::Vec3f(10, 1, 10), 
        VCore::Math::Vec3f::FRONT,
        VCore::Math::Vec3f::UP.cross(VCore::Math::Vec3f::FRONT),
        VCore::Math::Vec3f::RIGHT.cross(VCore::Math::Vec3f::FRONT),
        1920.f / 1080.f, 
        90.f * (M_PI / 180.f), 
        0.05f, 
        1000.f
    );

    auto model = std::make_shared<VCore::CVoxelSpace>();

    for (uint32_t x = 0; x < VCore::Config::ChunkSize * 3; x++) 
    {
        for (uint32_t y = 0; y < VCore::Config::ChunkSize * 3; y++) 
        {
            for (uint32_t z = 0; z < VCore::Config::ChunkSize * 3; z++) 
            {
                model->insert({VCore::Math::Vec3i(x, y, z), VCore::CVoxel(0, 0)});
            }
        }
    }

    auto query = model->queryChunks(&frustum);
    
    auto counter = 0;
    for (auto &&chunk : query) 
        counter++;
    CHECK_NE(counter, 0);
}