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

namespace VCore
{
    namespace MaterialManager
    {
        struct SMaterialSlot
        {
            SMaterialSlot() : Allocated(false) {}

            bool Allocated;
            CMaterial Material;
        };

        static constexpr uint8_t MaxSlots = (Config::MaxMaterialSlots == UCHAR_MAX) ? (Config::MaxMaterialSlots - 1) : Config::MaxMaterialSlots;

        // All allocated materials.
        static SMaterialSlot g_Slots[MaxSlots];

        // Slot zero is the default material
        static SMaterialSlot *g_NextFreeSlot = g_Slots + 1;

        uint8_t AddMaterial(const CMaterial &_Material)
        {
            auto slot = CreateMaterial();
            if(slot != UCHAR_MAX) 
                g_Slots[slot].Material = _Material;

            return slot;
        }

        CMaterial *GetMaterial(const uint8_t _MaterialHandle)
        {
            if(_MaterialHandle >= MaxSlots)
                return nullptr;

            auto &slot = g_Slots[_MaterialHandle];
            if(slot.Allocated || _MaterialHandle == 0)
                return &slot.Material;

            return nullptr;
        }
    	
        uint8_t CreateMaterial()
        {
            if(g_NextFreeSlot >= g_Slots + MaxSlots)
                return UCHAR_MAX;

            while (g_NextFreeSlot->Allocated && (g_NextFreeSlot < g_Slots + MaxSlots))
                g_NextFreeSlot++;
            
            // No more free slots
            if(g_NextFreeSlot->Allocated)
                return UCHAR_MAX;

            g_NextFreeSlot->Allocated = true;
            auto index = g_NextFreeSlot - g_Slots;
            g_NextFreeSlot++;
            return index;
        }

        uint8_t FindMaterialSlot(const CMaterial &_Material)
        {
            for (size_t i = 0; i < MaxSlots; i++)
            {
                if(g_Slots[i].Allocated || (i == 0))
                {
                    if(g_Slots[i].Material == _Material)
                        return i;
                }
            }
            
            return UCHAR_MAX;
        }

        void DeleteMaterial(const uint8_t _MaterialHandle)
        {
            if(_MaterialHandle == 0 || _MaterialHandle >= MaxSlots)
                return;

            g_Slots[_MaterialHandle].Allocated = false;
            g_Slots[_MaterialHandle].Material = CMaterial();

            if(!g_NextFreeSlot || g_NextFreeSlot > (g_Slots + _MaterialHandle))
                g_NextFreeSlot = g_Slots + _MaterialHandle;
        }
    } // namespace MaterialManager
} // namespace VCore
