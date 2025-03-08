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

#include <VCore/Meshing/Material.hpp>

namespace VCore
{
    namespace MaterialManager
    {
        /**
         * @brief Adds a new material to the material manager
         * @return Returns a handle to the material or UCHAR_MAX, if all slots are in use.
         */
        extern uint8_t AddMaterial(const CMaterial &_Material);

        /**
         * @brief Adds a new material or gets the already existing handle of the material
         * @return Returns a handle to the material or UCHAR_MAX, if all slots are in use.
         */
        extern uint8_t AddOrGetMaterial(const CMaterial &_Material);

        /**
         * @brief Retrieves a material by it's handle.
         * @return Returns a reference to the material, which is associated with the handle, or null, if there is no material associated.
         */
        extern CMaterial *GetMaterial(const uint8_t _MaterialHandle);
    	
        /**
         * @brief Creates a new default material.
         * @return Returns a handle to the new material or UCHAR_MAX, if all slots are in use.
         */
        extern uint8_t CreateMaterial();

        /**
         * @brief Uses linear search for finding the slot for a given material.
         * @return Returns a handle to the material or UCHAR_MAX, if no slots was found.
         */
        extern uint8_t FindMaterialSlot(const CMaterial &_Material);

        /**
         * @brief Frees a material.
         */
        extern void DeleteMaterial(const uint8_t _MaterialHandle);
    } // namespace MaterialManager
} // namespace VCore
