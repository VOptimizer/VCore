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

#include "Slices.hpp"
#include <future>

namespace VCore
{
    void CSliceCollection::Optimize()
    {
        for (size_t runAxis = 0; runAxis < 3; runAxis++)
        {
            int heightAxis = (runAxis + 1) % 3;
            int widthAxis = (runAxis + 2) % 3;

            auto &slices = mSlices[runAxis];
            for (auto &&slice : slices) // std::map<int, Slice>;
            {
                // Runs the slice on it's horizontal axis.
                // Each neighboaring quad which shares the same properties are merged into one.
                for (auto &&height : slice.second)
                {
                    Quads newList;
                    size_t current = 0;
                    
                    for (size_t j = current + 1; j < height.second.size(); j++)
                    {
                        auto &quad = height.second[current];
                        auto &quad2 = height.second[j];

                        // Combine two quads into one, if these properties are the same.
                        if(((quad.mQuad.first.v[widthAxis] + quad.mQuad.second.v[widthAxis]) == quad2.mQuad.first.v[widthAxis]) &&
                           (quad.mQuad.second.v[heightAxis] == quad2.mQuad.second.v[heightAxis]) &&
                           (quad.Material == quad2.Material) && (quad.Color == quad2.Color))
                            quad.mQuad.second.v[widthAxis] += quad2.mQuad.second.v[widthAxis];
                        else
                        {
                            // Put the current quad in a new list.
                            newList.push_back(quad);

                            // Move to the new quad, which is different.
                            current = j;
                        }
                    }

                    // The last quad we had processed should be added to.
                    if(current < height.second.size())
                        newList.push_back(height.second[current]);

                    // Swap the current strip.
                    mSlices[runAxis][slice.first][height.first] = std::move(newList);
                }

                // Does the same as above, but vertically.
                for (auto &&height : slice.second)
                {
                    for (auto &&quad : height.second)
                    {
                        // Dangerous!!!
                        // Combines all quads in a column, which are sharing the same properties.
                        while (true)
                        {
                            int heightPos = quad.mQuad.first.v[heightAxis] + quad.mQuad.second.v[heightAxis];
                            auto pos = quad.mQuad.first;
                            pos.v[heightAxis] += quad.mQuad.second.v[heightAxis];

                            // Look above for a strip.
                            auto heightIt = slice.second.find(heightPos);
                            if(heightIt == slice.second.end())
                                break;

                            auto &quadList = heightIt->second;

                            // Find in the strip above the quad which is on our position.
                            auto it = FindQuad(quadList, pos);
                            if(it != quadList.end())
                            {
                                // Combine two quads into one.
                                if(quad.mQuad.second.v[widthAxis] == it->mQuad.second.v[widthAxis] &&
                                   quad.Material == it->Material && quad.Color == it->Color) 
                                {
                                    quad.mQuad.second.v[heightAxis] += it->mQuad.second.v[heightAxis];

                                    // Remove the quad.
                                    quadList.erase(it);
                                }
                                else
                                    break;
                            }
                            else
                                break;
                        }
                    }
                }
            }
        }
    }

    void CSliceCollection::Merge(const CSliceCollection &_Other)
    {
        for (size_t i = 0; i < 3; i++)
        {
            auto &slices = _Other.mSlices[i];
            for (auto &&slice : slices) // std::map<int, Slice>;
            {
                for (auto &&height : slice.second) // std::map<int, Quads>;
                {
                    auto it = FindInsertionPoint(mSlices[i][slice.first][height.first], height.second.front().mQuad.first);
                    mSlices[i][slice.first][height.first].insert(it, height.second.begin(), height.second.end());
                }
            }
        }
    }

    void CSliceCollection::AddSlice(int _Axis, int _Depth)
    {
        if(_Axis < 3)
            mSlices[_Axis].emplace(_Depth, Slice());
    }

    void CSliceCollection::AddQuadInfo(int _Axis, int _Depth, int _Height, const CQuadInfo &_Info)
    {
        if(_Axis < 3 && !mSlices[_Axis].empty())
            mSlices[_Axis][_Depth][_Height].push_back(_Info);
    }

    Quads::const_iterator CSliceCollection::FindInsertionPoint(const Quads &_Haystack, const Math::Vec3i _Pos)
    {
        if(_Haystack.empty())
            return _Haystack.end();
        
        int left = 0, right = _Haystack.size() - 1;
        while (left <= right)
        {
            int center = left + (right - left) / 2;
            if(_Haystack[center].mQuad.first == _Pos)
                return _Haystack.begin() + center;

            if(_Haystack[center].mQuad.first > _Pos)
                right = center - 1;
            else
                left = center + 1;
        }
        
        return _Haystack.begin() + left;
    }

    Quads::const_iterator CSliceCollection::FindQuad(const Quads &_Haystack, const Math::Vec3i _Pos)
    {
        int left = 0, right = _Haystack.size() - 1;
        while (left <= right)
        {
            int center = left + (right - left) / 2;
            if(_Haystack[center].mQuad.first == _Pos)
                return _Haystack.begin() + center;

            if(_Haystack[center].mQuad.first > _Pos)
                right = center - 1;
            else
                left = center + 1;
        }
        
        return _Haystack.end();
    }
} // namespace VCore
