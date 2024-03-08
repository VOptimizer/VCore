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

#include <VCore/Voxel/VoxelAnimation.hpp>

namespace VCore
{
    void CVoxelAnimation::AddFrame(VoxelModel _Model, unsigned int _FrameTime)
    {
        m_Frames.push_back({_Model, _FrameTime});
    }

    void CVoxelAnimation::RemoveFrame(size_t _Frame)
    {
        if(_Frame < m_Frames.size())
            m_Frames.erase(m_Frames.begin() + _Frame);
    }

    SVoxelFrame CVoxelAnimation::GetFrame(size_t _Frame) const
    {
        SVoxelFrame result(nullptr, 0);
        if(_Frame < m_Frames.size())
            result = m_Frames[_Frame];

        return result;
    }
} // namespace VCore
