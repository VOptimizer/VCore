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
        if(topsize.IsZero())
            return;

        m_Mesh->Clear();
        m_Mesh->Colorpalettes()[TextureType::DIFFIUSE] = std::make_shared<CTexture>();

        ProjectPlane(_planes, _info.Top, 2);
        if(!frontsize.IsZero())
            ProjectPlane(_planes, _info.Front, 1, ProjectionMode::SUBSTRACT);

        m_Mesh->GetVoxels().generateVisibilityMask();
    }

    void CPlanesVoxelizer::ProjectPlane(Texture _planes, const CBBox &_bbox, char _axis, ProjectionMode _pmode)
    {
        CVectori size = _bbox.GetSize() - CVectori(1, 1, 1);
        std::vector<CVoxel::Visibility> mask(size.x * size.y, CVoxel::Visibility::VISIBLE);
        char axis1 = (_axis + 1) % 3; // 1 = 1 = y, 2 = 2 = z, 3 = 0 = x
        char axis2 = (_axis + 2) % 3; // 2 = 2 = z, 3 = 0 = x, 4 = 1 = y

        for (int h = 0; h < m_Mesh->GetSize().v[_axis]; h++)
        {
            for (int y = 0; y < size.y; y++)
            {
                for (int x = 0; x < size.x; x++)
                {
                    CVectori pixelPos = CVectori(_bbox.Beg.x + x, _bbox.Beg.x + y, 0);

                    auto p = _planes->Pixel(pixelPos);
                    CColor c(p);
                    if(c.A == 0)
                    {
                        mask[x + size.x * y] = CVoxel::Visibility::INVISIBLE;
                        if((_pmode == ProjectionMode::ADD || _pmode == ProjectionMode::TEXTURE))  // Can't add or texturing with an invisible pixel
                            continue;
                    }
                    else if(h == 0) // Only the first time
                    {
                        CVectori nextPX = pixelPos + CVectori(1, 0, 0);
                        if(nextPX.x < size.x)
                        {
                            auto px = _planes->Pixel(nextPX);
                            CColor cx(px);

                            if(cx.A != 0)
                            {
                                mask[x + size.x * y] &= ~GetMask(_axis, CVoxel::Visibility::RIGHT);
                                mask[(x + 1) + size.x * y] &= ~GetMask(_axis, CVoxel::Visibility::LEFT);
                            }
                        }

                        CVectori nextPY = pixelPos + CVectori(0, 1, 0);
                        if(nextPY.y < size.y)
                        {
                            auto py = _planes->Pixel(nextPY);
                            CColor cy(py);

                            if(cy.A != 0)
                            {
                                mask[x + size.x * y] &= ~GetMask(_axis, CVoxel::Visibility::DOWN);
                                mask[x + size.x * (y + 1)] &= ~GetMask(_axis, CVoxel::Visibility::UP);
                            }
                        }
                    }

                    int colorIdx = 0;
                    // Checks if the color is already indexed.
                    auto it = m_ColorMapping.find(p);
                    if(it != m_ColorMapping.end())
                        colorIdx = it->second;
                    else
                    {
                        // Creates a new index for this color.
                        colorIdx = m_Mesh->Colorpalettes()[TextureType::DIFFIUSE]->Size().x;
                        m_Mesh->Colorpalettes()[TextureType::DIFFIUSE]->AddPixel(c);
                        m_ColorMapping.insert({p, colorIdx});
                    }

                    CVector pos;
                    pos.v[axis1] = x;
                    pos.v[axis2] = y;
                    pos.v[_axis] = h;

                    switch (_pmode)
                    {
                        case ProjectionMode::ADD:
                        {
                            auto m = mask[x + size.x * y];
                            if(m_Mesh->GetSize().v[_axis] > 1)
                            {
                                if(h == 0)
                                    m &= ~GetMask(_axis, CVoxel::Visibility::FORWARD);
                                else if((h + 1) == m_Mesh->GetSize().v[_axis])
                                    m &= ~GetMask(_axis, CVoxel::Visibility::BACKWARD);
                                else
                                {
                                    m &= ~GetMask(_axis, CVoxel::Visibility::FORWARD);
                                    m &= ~GetMask(_axis, CVoxel::Visibility::BACKWARD);
                                }
                            }

                            m_Mesh->SetVoxel(pos, 0, colorIdx, false);
                        } break;

                        case ProjectionMode::SUBSTRACT:
                        {
                            auto m = mask[x + size.x * y];
                            if(m == CVoxel::Visibility::INVISIBLE)
                                m_Mesh->RemoveVoxel(pos);
                        }   // Intentional "forgotten" break

                        case ProjectionMode::TEXTURE:
                        {
                            
                        } break;
                    }
                }
            }
        }
    }

    CVoxel::Visibility CPlanesVoxelizer::GetMask(char _axis, CVoxel::Visibility _side)
    {
        // Maps the requested side relative to the viewing angle.
        switch (_side)
        {
            case CVoxel::Visibility::LEFT:
            {
                if(_axis == 2 || _axis == 1)    // Top and Front
                    return _side;
                else
                    return CVoxel::Visibility::FORWARD;
            } break;

            case CVoxel::Visibility::RIGHT:
            {
                if(_axis == 2 || _axis == 1)    // Top and Front
                    return _side;
                else
                    return CVoxel::Visibility::BACKWARD;
            } break;

            case CVoxel::Visibility::UP:
            {
                if(_axis == 0 || _axis == 1)    // Side and Front
                    return _side;
                else
                    return CVoxel::Visibility::BACKWARD;
            } break;

            case CVoxel::Visibility::DOWN:
            {
                if(_axis == 2 || _axis == 1)    // Side and Front
                    return _side;
                else
                    return CVoxel::Visibility::FORWARD;
            } break;

            case CVoxel::Visibility::FORWARD:
            {
                if(_axis == 0)
                    return _side;
                else if(_axis == 1)
                    return CVoxel::Visibility::LEFT;
                else if(_axis == 2)
                    return CVoxel::Visibility::UP;
            } break;

            case CVoxel::Visibility::BACKWARD:
            {
                if(_axis == 0)
                    return _side;
                else if(_axis == 1)
                    return CVoxel::Visibility::RIGHT;
                else if(_axis == 2)
                    return CVoxel::Visibility::DOWN;
            } break;
        }
    }
} // namespace VoxelOptimizer
