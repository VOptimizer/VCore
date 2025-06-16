#include "VCore/Meshing/Material.hpp"
#include "VCore/VConfig.hpp"
#include "StringMakerSpecializations.hpp"

#include <VCore/Meshing/MaterialManager.hpp>
#include <climits>
#include <cstdint>
#include <vector>

TEST_CASE("material handle 0 -> default material")
{
    auto material = VCore::MaterialManager::GetMaterial(0);
    REQUIRE(material != nullptr);

    CHECK(*material == VCore::CMaterial());
}

TEST_CASE("material handle 2 -> invalid")
{
    auto material = VCore::MaterialManager::GetMaterial(2);
    CHECK(material == nullptr);
}

TEST_CASE("add or get material -> default -> get handle 0")
{
    auto handle = VCore::MaterialManager::AddOrGetMaterial(VCore::CMaterial());
    CHECK(handle == 0);
}

TEST_CASE("add or get material -> none default -> get handle !0")
{
    VCore::CMaterial material;
    material.Emission = 0.5f;

    auto handle = VCore::MaterialManager::AddOrGetMaterial(material);
    CHECK(handle != 0);
    CHECK(handle != UCHAR_MAX);

    VCore::MaterialManager::DeleteMaterial(handle);
}

TEST_CASE("add material and delete it -> try to get = invalid")
{
    auto handle = VCore::MaterialManager::AddMaterial(VCore::CMaterial());
    CHECK(handle != 0);
    CHECK(handle != UCHAR_MAX);
    
    auto material = VCore::MaterialManager::GetMaterial(handle);
    CHECK(material != nullptr);

    // Delete material
    VCore::MaterialManager::DeleteMaterial(handle);
    material = VCore::MaterialManager::GetMaterial(handle);
    CHECK(material == nullptr);
}

TEST_CASE("find material -> default")
{
    auto foundhandle = VCore::MaterialManager::FindMaterialSlot(VCore::CMaterial());
    CHECK(foundhandle == 0);
}

TEST_CASE("find material -> none default")
{
    VCore::CMaterial material;
    material.Emission = 0.5f;

    auto handle = VCore::MaterialManager::AddOrGetMaterial(material);

    auto foundhandle = VCore::MaterialManager::FindMaterialSlot(material);
    CHECK(handle == foundhandle);

    VCore::MaterialManager::DeleteMaterial(foundhandle);
}

TEST_CASE("find material -> not exists")
{
    VCore::CMaterial material;
    material.Emission = 0.5f;

    auto foundhandle = VCore::MaterialManager::FindMaterialSlot(material);
    CHECK(foundhandle == UCHAR_MAX);
}

TEST_CASE("delete none existing material -> material 5")
{
    VCore::MaterialManager::DeleteMaterial(5);   
}

TEST_CASE("full material list -> expect UCHAR_MAX for the overflow one")
{
    std::vector<uint8_t> materialHandles = { 0 };

    // Fill all slots with materials.
    for (uint8_t i; i < VCore::Config::MaxMaterialSlots - 2; i++) 
        materialHandles.push_back(VCore::MaterialManager::AddMaterial(VCore::CMaterial()));

    CHECK(materialHandles.size() == VCore::Config::MaxMaterialSlots - 1);
    CHECK(materialHandles.back() != UCHAR_MAX);

    // Add another material, this time all slots should be full.
    auto overflowHandle = VCore::MaterialManager::AddMaterial(VCore::CMaterial());
    CHECK(overflowHandle == UCHAR_MAX);

    for (auto handle : materialHandles)
        VCore::MaterialManager::DeleteMaterial(handle);
}