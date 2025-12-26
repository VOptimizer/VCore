/*
 * MIT License
 *
 * Copyright (c) 2023 Christian Tost
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

#ifndef VOXEL_HPP
#define VOXEL_HPP

#include <stdlib.h>
#include <stdint.h>
#include <VCore/Math/Vector.hpp>

namespace VCore
{
    class CVoxel
    {
        public:
            CVoxel() : m_Value(0xFFFFFFFF) { }
            CVoxel(uint32_t p_Color, uint32_t p_Material) : m_Value((p_Color & 0xFFFFFF) | ((p_Material & 0xFF) << 24)) { }
            CVoxel(const CVoxel &p_Other) { *this = p_Other; }

            inline uint8_t GetMaterial() const { return (m_Value >> 24) & 0xFF; }
            inline uint32_t GetColor() const { return m_Value & 0xFFFFFF; }

            inline CVoxel &operator=(const CVoxel &p_Other)
            {
                m_Value = p_Other.m_Value;
                return *this;
            }

            /**
             * @return Returns true if this voxel is instantiated.
             */
            inline bool IsInstantiated() const
            {
                return m_Value != 0xFFFFFFFF;
            }

            inline operator uint32_t() const
            {
                return m_Value;
            };

            inline bool operator==(const CVoxel &p_rhs) const
            {
                return m_Value == p_rhs.m_Value;
            }

            inline bool operator!=(const CVoxel &p_rhs) const
            {
                return m_Value != p_rhs.m_Value;
            }

            ~CVoxel() = default;

        private:
            // m_Value combines the color and the material of a voxel
            // The top most byte is the material index and the rest 
            // is the RGB color of the voxel.
            uint32_t m_Value;
    };
}

#endif