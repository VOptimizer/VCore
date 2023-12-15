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
            enum Visibility : uint8_t
            {
                INVISIBLE = 0,
                UP = 1,
                DOWN = 2,
                LEFT = 4,
                RIGHT = 8,
                FORWARD = 16,
                BACKWARD = 32,

                VISIBLE = (UP | DOWN | LEFT | RIGHT | FORWARD | BACKWARD)
            };

            CVoxel();

            int Color;      //!< Index of the color.
            short Material;   //!< Index of the material.
            
            Visibility VisibilityMask;
            bool Transparent;

            /**
             * @return Returns true if at least one side is visible.
             */
            bool IsVisible() const;

            /**
             * @return Returns true if this voxel is instantiated.
             */
            bool IsInstantiated() const;

            ~CVoxel() = default;
    };

    using Voxel = CVoxel*;

    //////////////////////////////////////////////////
    // CVoxel functions
    //////////////////////////////////////////////////

    inline CVoxel::CVoxel() : Color(-1), Material(-1), Transparent(false)
    {
        VisibilityMask = Visibility::INVISIBLE;
    }

    inline bool CVoxel::IsVisible() const
    {                
        return IsInstantiated() && (VisibilityMask != Visibility::INVISIBLE);
    }

    inline bool CVoxel::IsInstantiated() const
    {
        return (Color != -1) && (Material != -1);
    }

    //////////////////////////////////////////////////
    // CVoxel::Visibility functions
    //////////////////////////////////////////////////

    inline CVoxel::Visibility operator&(CVoxel::Visibility lhs, CVoxel::Visibility rhs)
    {
        return static_cast<CVoxel::Visibility>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
    }

    inline CVoxel::Visibility operator&=(CVoxel::Visibility &lhs, const CVoxel::Visibility rhs)
    {
        lhs = static_cast<CVoxel::Visibility>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
        return lhs;
    }

    inline CVoxel::Visibility operator|=(CVoxel::Visibility &lhs, const CVoxel::Visibility rhs)
    {
        lhs = static_cast<CVoxel::Visibility>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
        return lhs;
    }

    inline CVoxel::Visibility operator~(CVoxel::Visibility lhs)
    {
        return static_cast<CVoxel::Visibility>(~static_cast<uint8_t>(lhs));
    }
}

#endif