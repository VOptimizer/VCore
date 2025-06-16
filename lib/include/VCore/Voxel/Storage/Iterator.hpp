/*
 * MIT License
 *
 * Copyright (c) 2024 Christian Tost
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

#ifndef ITERATORS_HPP
#define ITERATORS_HPP

#include <VCore/Voxel/BBox.hpp>
#include "../Voxel.hpp"
#include <utility>
#include <VCore/VConfig.hpp>

namespace VCore
{
    class CVoxelSpace;
    class CVoxelSpaceIterator
    {
        public:
            using pair = std::pair<Math::Vec3i, CVoxel>;
            using reference = std::pair<Math::Vec3i, CVoxel>&;
            using pointer = pair*;

            CVoxelSpaceIterator();
            CVoxelSpaceIterator(const CVoxelSpace *p_Space, const CBBox &p_InnerBox, const pair &p_Pair);
            CVoxelSpaceIterator(const CVoxelSpaceIterator &p_Other);
            CVoxelSpaceIterator(CVoxelSpaceIterator &&p_Other);

            reference operator*() const;
            pointer operator->() const;

            CVoxelSpaceIterator& operator++();
            CVoxelSpaceIterator& operator++(int);

            bool operator!=(const CVoxelSpaceIterator &p_Rhs) const;
            bool operator==(const CVoxelSpaceIterator &p_Rhs) const;

            CVoxelSpaceIterator& operator=(const CVoxelSpaceIterator &p_Other);
            CVoxelSpaceIterator& operator=(CVoxelSpaceIterator &&p_Other);
        
        private:
            const CVoxelSpace *m_Space;
            CBBox m_InnerBox;
            mutable pair m_Pair;
    };
} // namespace VCore


#endif