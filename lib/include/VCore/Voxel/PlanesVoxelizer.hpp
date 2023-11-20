/*
* MIT License
*
* Copyright (c) 2022 Christian Tost
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

#ifndef PLANESVOXELIZER_HPP
#define PLANESVOXELIZER_HPP

#include <VCore/Voxel/VoxelModel.hpp>

namespace VCore
{
    struct SPlanesInfo
    {
        CBBox Top;
        CBBox Bottom;
        CBBox Left;
        CBBox Right;
        CBBox Front;
        CBBox Back;
    };
    
    class CPlanesVoxelizer
    {
        public:
            CPlanesVoxelizer();

            /**
             * @brief Sets the size for the voxel space.
             */
            void SetVoxelSpaceSize(const Math::Vec3i &_size);

            /**
             * @return Returns the mesh which is managed by this instance.
             */
            VoxelModel GetMesh();

            /**
             * @brief Updates the mesh with the given planes.
             * 
             * @param _planes: Textureatlas of informations
             * @param _info: Info where in the atlas a specific plane is stored
             */
            void UpdateMesh(Texture _planes, const SPlanesInfo &_info);

            ~CPlanesVoxelizer() = default;
        private:
            void ProjectPlanes(Texture _planes, const SPlanesInfo &_info);
            void ProjectTexture(Texture _planes, const CBBox &_bbox, uint8_t _axis, bool _otherSide = false);

            int AddOrGetColor(uint32_t _color);
            CColor GetColor(Texture _planes, const Math::Vec2ui &_pos);

            VoxelModel m_Mesh;
            std::map<uint32_t, int, std::less<uint32_t>> m_ColorMapping;
    };
}

#endif