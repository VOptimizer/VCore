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

#include <VCore/Voxel/Storage/Iterator.hpp>
#include <VCore/Voxel/Storage/VoxelSpace.hpp>

namespace VCore
{
    //////////////////////////////////////////////////
    // CVoxelSpaceIterator functions
    //////////////////////////////////////////////////

    CVoxelSpaceIterator::CVoxelSpaceIterator() : m_Space(nullptr), m_Pair(Math::Vec3i(), CVoxel()) { }
    CVoxelSpaceIterator::CVoxelSpaceIterator(const CVoxelSpace *p_Space, const CBBox &p_InnerBox, const pair &p_Pair) : m_Space(p_Space), m_InnerBox(p_InnerBox), m_Pair(p_Pair) { }
    CVoxelSpaceIterator::CVoxelSpaceIterator(const CVoxelSpaceIterator &p_Other)
    {
        *this = p_Other;
    }

    CVoxelSpaceIterator::CVoxelSpaceIterator(CVoxelSpaceIterator &&p_Other)
    {
        *this = std::move(p_Other);
    }

    typename CVoxelSpaceIterator::reference CVoxelSpaceIterator::operator*() const
    {
        return m_Pair;
    }

    typename CVoxelSpaceIterator::pointer CVoxelSpaceIterator::operator->() const
    {
        return &m_Pair;
    }
    
    CVoxelSpaceIterator& CVoxelSpaceIterator::operator++()
    {
        auto next = m_Pair.first;
        next.x++;

        if(next.x > m_InnerBox.End.x)
        {
            next.x = m_InnerBox.Beg.x;
            next.y++;
            if(next.y > m_InnerBox.End.y)
            {
                next.y = m_InnerBox.Beg.y;
                next.z++;
            }
        }

        *this = m_Space->next(next);
        return *this;
    }

    CVoxelSpaceIterator& CVoxelSpaceIterator::operator++(int)
    {
        auto next = m_Pair.first;
        next.x++;

        if(next.x > m_InnerBox.End.x)
        {
            next.x = m_InnerBox.Beg.x;
            next.y++;
            if(next.y > m_InnerBox.End.y)
            {
                next.y = m_InnerBox.Beg.y;
                next.z++;
            }
        }

        *this = m_Space->next(next);
        return *this;
    }

    bool CVoxelSpaceIterator::operator!=(const CVoxelSpaceIterator &p_Rhs) const
    {
        return (m_Space != p_Rhs.m_Space) || (m_Pair.first != p_Rhs.m_Pair.first) || (m_Pair.second != p_Rhs.m_Pair.second);
    }

    bool CVoxelSpaceIterator::operator==(const CVoxelSpaceIterator &p_Rhs) const
    {
        return (m_Space == p_Rhs.m_Space) && (m_Pair.first == p_Rhs.m_Pair.first) && (m_Pair.second == p_Rhs.m_Pair.second);
    }

    CVoxelSpaceIterator& CVoxelSpaceIterator::operator=(const CVoxelSpaceIterator &p_Other)
    {
        m_Space = p_Other.m_Space;
        m_Pair = p_Other.m_Pair;
        m_InnerBox = p_Other.m_InnerBox;

        return *this;
    }

    CVoxelSpaceIterator& CVoxelSpaceIterator::operator=(CVoxelSpaceIterator &&p_Other)
    {
        m_Space = p_Other.m_Space;
        m_Pair = p_Other.m_Pair;
        m_InnerBox = p_Other.m_InnerBox;

        p_Other.m_Space = nullptr;
        p_Other.m_Pair = {Math::Vec3i(), CVoxel()};
        p_Other.m_InnerBox = CBBox();

        return *this;
    }
} // namespace VCore
