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

#include <VoxelOptimizer/Voxel/PlanesVoxelizer.hpp>

namespace VoxelOptimizer
{
    CPlanesVoxelizer::CPlanesVoxelizer()
    {
        m_Mesh = std::make_shared<CVoxelMesh>();
        m_Mesh->Materials().push_back(std::make_shared<CMaterial>());
    }

    void CPlanesVoxelizer::SetVoxelSpaceSize(const CVectori &_size)
    {
        m_Mesh->SetSize(_size);
        m_Mesh->SetBBox(CBBox(CVectori(), m_Mesh->GetSize()));
    }

    VoxelMesh CPlanesVoxelizer::GetMesh()
    {
        return m_Mesh;
    }

    void CPlanesVoxelizer::UpdateMesh(Texture _planes, const SPlanesInfo &_info)
    {
        m_ColorMapping.clear();
        auto topsize = _info.Top.GetSize() - CVector(1, 1, 1);
        auto frontsize = _info.Front.GetSize() - CVector(1, 1, 1);
        auto leftsize = _info.Left.GetSize() - CVector(1, 1, 1);
        auto rightsize = _info.Right.GetSize() - CVector(1, 1, 1);
        auto bottomsize = _info.Bottom.GetSize() - CVector(1, 1, 1);
        auto backsize = _info.Back.GetSize() - CVector(1, 1, 1);
        if(topsize.IsZero())
            return;

        m_Mesh->Clear();
        m_Mesh->Colorpalettes()[TextureType::DIFFIUSE] = std::make_shared<CTexture>();

        ProjectPlanes(_planes, _info);
        if(!frontsize.IsZero())
            ProjectTexture(_planes, _info.Front, 1);

        if(!leftsize.IsZero())
            ProjectTexture(_planes, _info.Left, 0);

        m_Mesh->GetVoxels().generateVisibilityMask();
    }

    void CPlanesVoxelizer::ProjectPlanes(Texture _planes, const SPlanesInfo &_info)
    {
        CVectori sizeTop = _info.Top.GetSize() - CVectori(1, 1, 1);
        CVectori sizeFront = _info.Front.GetSize() - CVectori(1, 1, 1);
        CVectori sizeLeft = _info.Left.GetSize() - CVectori(1, 1, 1);

        for (int z = 0; z < m_Mesh->GetSize().z; z++)
        {
            for (int y = 0; y < sizeTop.y; y++)
            {
                for (int x = 0; x < sizeTop.x; x++)
                {
                    // Checks if the current voxel needs to be added.
                    CColor c = GetColor(_planes, CVectori(_info.Top.Beg.x + x, _info.Top.Beg.y + y, 0));
                    if(c.A == 0) // Can't add an invisible pixel  
                        continue;

                    if(!sizeFront.IsZero())
                    {
                        CColor cf = GetColor(_planes, CVectori(_info.Front.Beg.x + x, _info.Front.Beg.y + (m_Mesh->GetSize().z - z - 1), 0));
                        if(cf.A == 0) // Can't add an invisible pixel  
                            continue;
                    }

                    if(!sizeLeft.IsZero())
                    {
                        CColor cl = GetColor(_planes, CVectori(_info.Left.Beg.x + y, _info.Left.Beg.y + (m_Mesh->GetSize().z - z - 1), 0));
                        if(cl.A == 0) // Can't add an invisible pixel  
                            continue;
                    }

                    int colorIdx = AddOrGetColor(c.AsRGBA());
                    CVectori pos(x, y, z);

                    m_Mesh->SetVoxel(pos, 0, colorIdx, false);
                }
            }
        }
    }

    void CPlanesVoxelizer::ProjectTexture(Texture _planes, const CBBox &_bbox, char _axis)
    {
        CVectori size = _bbox.GetSize() - CVectori(1, 1, 1);
        char axis1 = (_axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
        char axis2 = (_axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y

        for (int y = 0; y < size.y; y++)
        {
            for (int x = 0; x < size.x; x++)
            {
                auto p = _planes->Pixel(CVectori(_bbox.Beg.x + x, _bbox.Beg.y + (size.y - y - 1), 0));
                CColor c(p);
                if(c.A == 0)
                    continue;

                int colorIdx = AddOrGetColor(p);
                for (size_t z = 0; z < m_Mesh->GetSize().v[_axis]; z++)
                {
                    CVector pos;
                    if(_axis == 1)
                    {
                        pos.v[axis2] = x;
                        pos.v[axis1] = y;
                    }
                    else
                    {
                        pos.v[axis1] = x;
                        pos.v[axis2] = y;
                    }
                    pos.v[_axis] = z;

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
            colorIdx = m_Mesh->Colorpalettes()[TextureType::DIFFIUSE]->Size().x;
            m_Mesh->Colorpalettes()[TextureType::DIFFIUSE]->AddPixel(CColor(_color));
            m_ColorMapping.insert({_color, colorIdx});
        }

        return colorIdx;
    }

    CColor CPlanesVoxelizer::GetColor(Texture _planes, const CVectori &_pos)
    {
        auto p = _planes->Pixel(_pos);
        return CColor(p);
    }
} // namespace VoxelOptimizer
