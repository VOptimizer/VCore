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

#include <VCore/Voxel/PlanesVoxelizer.hpp>

namespace VCore
{
    CPlanesVoxelizer::CPlanesVoxelizer()
    {
        m_Mesh = std::make_shared<CVoxelMesh>();
        m_Mesh->Materials.push_back(std::make_shared<CMaterial>());
    }

    void CPlanesVoxelizer::SetVoxelSpaceSize(const Math::Vec3i &_size)
    {
        m_Mesh->SetSize(_size);
        m_Mesh->BBox = CBBox(Math::Vec3i(), m_Mesh->GetSize());
    }

    VoxelMesh CPlanesVoxelizer::GetMesh()
    {
        return m_Mesh;
    }

    void CPlanesVoxelizer::UpdateMesh(Texture _planes, const SPlanesInfo &_info)
    {
        m_ColorMapping.clear();
        auto topsize = _info.Top.GetSize() - Math::Vec3f(1, 1, 1);
        auto frontsize = _info.Front.GetSize() - Math::Vec3f(1, 1, 1);
        auto leftsize = _info.Left.GetSize() - Math::Vec3f(1, 1, 1);
        auto rightsize = _info.Right.GetSize() - Math::Vec3f(1, 1, 1);
        auto bottomsize = _info.Bottom.GetSize() - Math::Vec3f(1, 1, 1);
        auto backsize = _info.Back.GetSize() - Math::Vec3f(1, 1, 1);
        if(topsize.IsZero())
            return;

        m_Mesh->Clear();
        m_Mesh->Textures[TextureType::DIFFIUSE] = std::make_shared<CTexture>();

        ProjectPlanes(_planes, _info);
        if(!frontsize.IsZero())
            ProjectTexture(_planes, _info.Front, 1);

        if(!leftsize.IsZero())
            ProjectTexture(_planes, _info.Left, 0);

        if(!bottomsize.IsZero())
            ProjectTexture(_planes, _info.Bottom, 2);   // Bottom and top is reversed.

        if(!backsize.IsZero())
            ProjectTexture(_planes, _info.Back, 1, true);

        if(!rightsize.IsZero())
            ProjectTexture(_planes, _info.Right, 0, true);

        m_Mesh->GenerateVisibilityMask();
    }

    void CPlanesVoxelizer::ProjectPlanes(Texture _planes, const SPlanesInfo &_info)
    {
        Math::Vec3i sizeTop = _info.Top.GetSize() - Math::Vec3i(1, 1, 1);
        Math::Vec3i sizeFront = _info.Front.GetSize() - Math::Vec3i(1, 1, 1);
        Math::Vec3i sizeLeft = _info.Left.GetSize() - Math::Vec3i(1, 1, 1);

        for (int z = 0; z < m_Mesh->GetSize().z; z++)
        {
            for (int y = 0; y < sizeTop.y; y++)
            {
                for (int x = 0; x < sizeTop.x; x++)
                {
                    // Checks if the current voxel needs to be added.
                    CColor c = GetColor(_planes, Math::Vec2ui(_info.Top.Beg.x + x, _info.Top.Beg.y + y));
                    if(c.A == 0) // Can't add an invisible pixel  
                        continue;

                    if(!sizeFront.IsZero())
                    {
                        CColor cf = GetColor(_planes, Math::Vec2ui(_info.Front.Beg.x + x, _info.Front.Beg.y + (m_Mesh->GetSize().z - z - 1)));
                        if(cf.A == 0) // Can't add an invisible pixel  
                            continue;
                    }

                    if(!sizeLeft.IsZero())
                    {
                        CColor cl = GetColor(_planes, Math::Vec2ui(_info.Left.Beg.x + y, _info.Left.Beg.y + (m_Mesh->GetSize().z - z - 1)));
                        if(cl.A == 0) // Can't add an invisible pixel  
                            continue;
                    }

                    int colorIdx = AddOrGetColor(c.AsRGBA());
                    Math::Vec3i pos(x, sizeTop.y - y - 1, z);

                    m_Mesh->SetVoxel(pos, 0, colorIdx, false);
                }
            }
        }
    }

    void CPlanesVoxelizer::ProjectTexture(Texture _planes, const CBBox &_bbox, uint8_t _axis, bool _otherSide)
    {
        Math::Vec3i size = _bbox.GetSize() - Math::Vec3i(1, 1, 1);
        uint8_t axis1 = (_axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
        uint8_t axis2 = (_axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y

        for (int y = 0; y < size.y; y++)
        {
            for (int x = 0; x < size.x; x++)
            {
                // Gets the color of the current pixel.
                auto p = _planes->GetPixel(Math::Vec2ui(_bbox.Beg.x + x, _bbox.Beg.y + (size.y - y - 1)));
                CColor c(p);
                if(c.A == 0)    // Ignore invisible pixels.
                    continue;

                int colorIdx = AddOrGetColor(p);

                // Projects the color onto the mesh.
                for (int z = 0; z < m_Mesh->GetSize().v[_axis]; z++)
                {
                    // TODO: UGLY, but functional :(
                    Math::Vec3f pos;
                    if(_axis == 1)
                    {
                        if(_otherSide)
                            pos.v[axis2] = size.x - x - 1;
                        else
                            pos.v[axis2] = x;

                        pos.v[axis1] = y;
                    }
                    else
                    {
                        if((_otherSide && _axis != 0) || (!_otherSide && _axis == 0))
                            pos.v[axis1] = size.x - x - 1;
                        else
                            pos.v[axis1] = x;

                        if(_axis == 2)
                            pos.v[axis2] = size.y - y - 1;
                        else
                            pos.v[axis2] = y;
                    }

                    if(!_otherSide)
                        pos.v[_axis] = z;
                    else
                        pos.v[_axis] = m_Mesh->GetSize().v[_axis] - z - 1;

                    auto voxel = m_Mesh->GetVoxel(pos);
                    if(voxel)
                    {
                        voxel->Color = colorIdx;
                        break;
                    }
                }
            }
        } 
    }

    int CPlanesVoxelizer::AddOrGetColor(uint32_t _color)
    {
        int colorIdx = 0;

        // Checks if the color is already indexed.
        auto it = m_ColorMapping.find(_color);
        if(it != m_ColorMapping.end())
            colorIdx = it->second;
        else
        {
            // Creates a new index for this color.
            colorIdx = m_Mesh->Textures[TextureType::DIFFIUSE]->GetSize().x;
            m_Mesh->Textures[TextureType::DIFFIUSE]->AddPixel(CColor(_color));
            m_ColorMapping.insert({_color, colorIdx});
        }

        return colorIdx;
    }

    CColor CPlanesVoxelizer::GetColor(Texture _planes, const Math::Vec2ui &_pos)
    {
        auto p = _planes->GetPixel(_pos);
        return CColor(p);
    }
}
