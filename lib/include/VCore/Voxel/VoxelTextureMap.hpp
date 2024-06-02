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

#ifndef VOXELINFO_HPP
#define VOXELINFO_HPP

#include <VCore/Misc/unordered_dense.h>
#include <VCore/Math/Vector.hpp>

namespace VCore
{
    struct SUVMapping
    {
        SUVMapping() = default;
        SUVMapping(const Math::Vec2f &_TopLeft, const Math::Vec2f &_TopRight, const Math::Vec2f &_BottomLeft, const Math::Vec2f &_BottomRight) : TopLeft(_TopLeft), TopRight(_TopRight), BottomLeft(_BottomLeft), BottomRight(_BottomRight) {}

        Math::Vec2f TopLeft;
        Math::Vec2f TopRight;
        Math::Vec2f BottomLeft;
        Math::Vec2f BottomRight;
    };
    
    class CVoxelInfo
    {
        public:
            CVoxelInfo() = default;
            CVoxelInfo(const CVoxelInfo &_Other) { *this = _Other; }
            CVoxelInfo(CVoxelInfo &&_Other) { *this = std::move(_Other); }

            /**
             * @brief Adds a uv map for a given face.
             * 
             * @param _Normal: Normal of the face.
             * @param _UVMap: UV mapping of each vertex.
             */
            void AddFace(const Math::Vec3f &_Normal, const SUVMapping &_UVMap);

            /**
             * @return Gets the uvmap for a given face or null if the face doesn't have a uvmap.
             */
            const SUVMapping *GetUVMap(const Math::Vec3f &_Normal) const;

            CVoxelInfo &operator=(const CVoxelInfo &_Other);
            CVoxelInfo &operator=(CVoxelInfo &&_Other);

            ~CVoxelInfo() = default;

        private:
            VectorMap<SUVMapping> m_UVMap;
    };

    class CVoxelTextureMap
    {
        public:
            CVoxelTextureMap() = default;
            CVoxelTextureMap(const CVoxelTextureMap &_Other) { *this = _Other; }
            CVoxelTextureMap(CVoxelTextureMap &&_Other) { *this = std::move(_Other); }

            /**
             * @brief Adds a new info structure for a voxel type.
             * 
             * @param _Id: Id of the voxel (ColorIdx).
             * @param _Info: UV info of the voxel.
             */
            void AddVoxelInfo(int _Id, const CVoxelInfo &_Info);

            /**
             * @return Returns the texture info for a given face for a given voxel id or null if not found.
             */
            const SUVMapping *GetVoxelFaceInfo(int _Id, const Math::Vec3f &_Normal) const;

            CVoxelTextureMap &operator=(const CVoxelTextureMap &_Other);
            CVoxelTextureMap &operator=(CVoxelTextureMap &&_Other);

            ~CVoxelTextureMap() = default;
        private:
            ankerl::unordered_dense::map<int, CVoxelInfo> m_VoxelInfos;
    };
}


#endif