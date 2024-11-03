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

    CVoxelSpaceIterator::CVoxelSpaceIterator() : m_Space(nullptr), m_Pair(Math::Vec3i(), nullptr) { }
    CVoxelSpaceIterator::CVoxelSpaceIterator(const CVoxelSpace *_Space, const CBBox &_InnerBox, const pair &_Pair) : m_Space(_Space), m_InnerBox(_InnerBox), m_Pair(_Pair) { }
    CVoxelSpaceIterator::CVoxelSpaceIterator(const CVoxelSpaceIterator &_Other)
    {
        *this = _Other;
    }

    CVoxelSpaceIterator::CVoxelSpaceIterator(CVoxelSpaceIterator &&_Other)
    {
        *this = std::move(_Other);
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

    bool CVoxelSpaceIterator::operator!=(const CVoxelSpaceIterator &_Rhs)
    {
        return (m_Space != _Rhs.m_Space) || (m_Pair.first != _Rhs.m_Pair.first) || (m_Pair.second != _Rhs.m_Pair.second);
    }

    bool CVoxelSpaceIterator::operator==(const CVoxelSpaceIterator &_Rhs)
    {
        return (m_Space == _Rhs.m_Space) && (m_Pair.first == _Rhs.m_Pair.first) && (m_Pair.second == _Rhs.m_Pair.second);
    }

    CVoxelSpaceIterator& CVoxelSpaceIterator::operator=(const CVoxelSpaceIterator &_Other)
    {
        m_Space = _Other.m_Space;
        m_Pair = _Other.m_Pair;
        m_InnerBox = _Other.m_InnerBox;

        return *this;
    }

    CVoxelSpaceIterator& CVoxelSpaceIterator::operator=(CVoxelSpaceIterator &&_Other)
    {
        m_Space = _Other.m_Space;
        m_Pair = _Other.m_Pair;
        m_InnerBox = _Other.m_InnerBox;

        _Other.m_Space = nullptr;
        _Other.m_Pair = {Math::Vec3i(), nullptr};
        _Other.m_InnerBox = CBBox();

        return *this;
    }
} // namespace VCore
