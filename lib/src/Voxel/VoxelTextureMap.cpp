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

#include <VoxelOptimizer/Voxel/VoxelTextureMap.hpp>

namespace VoxelOptimizer
{
    //////////////////////////////////////////////////
    // CVoxelInfo functions
    //////////////////////////////////////////////////

    void CVoxelInfo::AddFace(const Math::Vec3f &_Normal, const SUVMapping &_UVMap)
    {
        m_UVMap[_Normal] = _UVMap;
    }

    const SUVMapping *CVoxelInfo::GetUVMap(const Math::Vec3f &_Normal) const
    {
        auto it = m_UVMap.find(_Normal);
        if(it == m_UVMap.end())
            return nullptr;

        return &it->second;
    }

    CVoxelInfo &CVoxelInfo::operator=(const CVoxelInfo &_Other)
    {
        m_UVMap = _Other.m_UVMap;
        return *this;
    }

    CVoxelInfo &CVoxelInfo::operator=(CVoxelInfo &&_Other)
    {
        m_UVMap = std::move(_Other.m_UVMap);
        return *this;
    }

    //////////////////////////////////////////////////
    // CVoxelTextureMap functions
    //////////////////////////////////////////////////

    void CVoxelTextureMap::AddVoxelInfo(int _Id, const CVoxelInfo &_Info)
    {
        m_VoxelInfos[_Id] = _Info;
    }

    const SUVMapping *CVoxelTextureMap::GetVoxelFaceInfo(int _Id, const Math::Vec3f &_Normal) const
    {
        auto it = m_VoxelInfos.find(_Id);
        if(it == m_VoxelInfos.end())
            return nullptr;

        return it->second.GetUVMap(_Normal);
    }

    CVoxelTextureMap &CVoxelTextureMap::operator=(const CVoxelTextureMap &_Other)
    {
        m_VoxelInfos = _Other.m_VoxelInfos;
        return *this;
    }

    CVoxelTextureMap &CVoxelTextureMap::operator=(CVoxelTextureMap &&_Other)
    {
        m_VoxelInfos = std::move(_Other.m_VoxelInfos);
        return *this;
    }
}
