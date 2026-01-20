/*
 * MIT License
 *
 * Copyright (c) 2025 Christian Tost
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <VCore/Meshing/MaterialManager.hpp>
#include <VCore/VConfig.hpp>
#include <climits>
#include <cstdint>

namespace VCore
{
    namespace MaterialManager
    {
        struct SMaterialSlot
        {
            SMaterialSlot() : Hash(0) {}

            uint64_t Hash;
            CMaterial Material;
        };

        static constexpr uint8_t MaxSlots = (Config::MaxMaterialSlots == UCHAR_MAX) ? (Config::MaxMaterialSlots - 1) : Config::MaxMaterialSlots;

        // All allocated materials.
        static SMaterialSlot g_Slots[MaxSlots];

        // Slot zero is the default material
        static SMaterialSlot *g_NextFreeSlot = g_Slots + 1;

        //////////////////////////////////////////////////
        // Internal functions
        //////////////////////////////////////////////////

        constexpr uint64_t HashMaterial(const CMaterial &p_Material)
        {
            auto rawBytes = (const char*)(&p_Material) + offsetof(CMaterial, Metallic);

            constexpr uint64_t magicPrime = 0x00000100000001b3;
            uint64_t hash = 0xcbf29ce484222325;

            for (uint32_t i = 0; i < sizeof(CMaterial) - offsetof(CMaterial, Metallic); i++)
                hash = (hash ^ rawBytes[i]) * magicPrime;

            return hash;
        }

        uint8_t FindMaterialSlot(const uint64_t p_Hash)
        {
            for (size_t i = 0; i < MaxSlots; i++)
            {
                if(g_Slots[i].Hash == p_Hash)
                    return i;
            }
            
            return UCHAR_MAX;
        }

        uint8_t FindMaterialSlot(const CMaterial &p_Material, uint64_t &p_Hash)
        {
            p_Hash = HashMaterial(p_Material);            
            // auto slot = FindMaterialSlot(p_Hash);
            // if(slot == UCHAR_MAX && g_Slots[0].Material == p_Material)
            //     return 0;

            // return slot;
            return FindMaterialSlot(p_Hash);
        }

        uint8_t CreateMaterial(const uint64_t p_Hash)
        {
            if(g_NextFreeSlot >= g_Slots + MaxSlots)
                return UCHAR_MAX;

            while (g_NextFreeSlot->Hash && (g_NextFreeSlot < g_Slots + MaxSlots))
                g_NextFreeSlot++;
            
            // No more free slots
            if(g_NextFreeSlot->Hash)
                return UCHAR_MAX;

            g_NextFreeSlot->Hash = p_Hash;
            auto index = g_NextFreeSlot - g_Slots;
            g_NextFreeSlot++;
            return index;
        }

        //////////////////////////////////////////////////
        // Public functions
        //////////////////////////////////////////////////

        uint8_t AddMaterial(const CMaterial &p_Material)
        {
            auto hash = HashMaterial(p_Material);
            auto slot = CreateMaterial(hash);
            if(slot != UCHAR_MAX) 
                g_Slots[slot].Material = p_Material;

            return slot;
        }

        uint8_t AddOrGetMaterial(const CMaterial &p_Material)
        {
            uint64_t hash;
            auto slot = FindMaterialSlot(p_Material, hash);
            if(slot == UCHAR_MAX)
            {
                slot = CreateMaterial(hash);
                if(slot != UCHAR_MAX) 
                    g_Slots[slot].Material = p_Material;
            }

            return slot;
        }

        CMaterial *GetMaterial(const uint8_t p_MaterialHandle)
        {
            if(p_MaterialHandle >= MaxSlots)
                return nullptr;

            auto &slot = g_Slots[p_MaterialHandle];
            if(slot.Hash || p_MaterialHandle == 0)
                return &slot.Material;

            return nullptr;
        }

        uint8_t FindMaterialSlot(const CMaterial &p_Material)
        {
            uint64_t hash;
            return FindMaterialSlot(p_Material, hash);
        }

        void DeleteMaterial(const uint8_t p_MaterialHandle)
        {
            if(p_MaterialHandle == 0 || p_MaterialHandle >= MaxSlots)
                return;

            g_Slots[p_MaterialHandle].Hash = 0;
            g_Slots[p_MaterialHandle].Material = CMaterial();

            if(!g_NextFreeSlot || g_NextFreeSlot > (g_Slots + p_MaterialHandle))
                g_NextFreeSlot = g_Slots + p_MaterialHandle;
        }

        class InitHack
        {
            public:
                InitHack() 
                { g_Slots[0].Hash = HashMaterial(g_Slots[0].Material); }
        };
        InitHack _;
    } // namespace MaterialManager
} // namespace VCore
