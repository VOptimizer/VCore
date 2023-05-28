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
#include <VoxelOptimizer/Math/Vector.hpp>

namespace VoxelOptimizer
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

            // Normal face directions inside the voxel space.
            const static CVector FACE_UP; // (0, 0, 1);
            const static CVector FACE_DOWN; // (0, 0, -1);
            const static CVector FACE_LEFT; // (-1, 0, 0);
            const static CVector FACE_RIGHT; // (1, 0, 0);
            const static CVector FACE_FORWARD; // (0, 1, 0);
            const static CVector FACE_BACKWARD; // (0, -1, 0);
            const static CVector FACE_ZERO; // (0, 0, 0);

            CVoxel();

            int Color;      //!< Index of the color.
            short Material;   //!< Index of the material.
            
            Visibility VisibilityMask;
            bool Transparent;

            /**
             * @return Returns true if at least one side is visible.
             */
            bool IsVisible() const;

            ~CVoxel() = default;
    };

    using Voxel = CVoxel*; //std::shared_ptr<CVoxel>;

    //////////////////////////////////////////////////
    // CVoxel functions
    //////////////////////////////////////////////////

    inline bool CVoxel::IsVisible() const
    {                
        return VisibilityMask != Visibility::INVISIBLE;
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
} // namespace VoxelOptimizer

#endif