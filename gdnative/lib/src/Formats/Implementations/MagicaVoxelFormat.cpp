/*
 * MIT License
 *
 * Copyright (c) 2021 Christian Tost
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

#include "MagicaVoxelFormat.hpp"
#include <string.h>
#include <sstream>
#include <stack>
#include <VoxelOptimizer/Exceptions.hpp>

namespace VoxelOptimizer
{
    // Copied from the official documentation. https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox.txt
    const static unsigned int default_palette[256] = {
        0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff,
        0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff, 0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff,
        0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff, 0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc,
        0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc, 0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc,
        0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc, 0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99,
        0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
        0xff006699, 0xffff3399, 0xffcc3399, 0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099, 0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66,
        0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66, 0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966, 0xff009966, 0xffff6666,
        0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366, 0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
        0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933,
        0xff669933, 0xff339933, 0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333, 0xff993333, 0xff663333, 0xff333333, 0xff003333, 0xffff0033,
        0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00,
        0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900, 0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300, 0xff993300,
        0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044,
        0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000,
        0xff880000, 0xff770000, 0xff550000, 0xff440000, 0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555, 0xff444444, 0xff222222, 0xff111111
    };

    void CMagicaVoxelFormat::ClearCache()
    {
        IVoxelFormat::ClearCache();
        LoadDefaultPalette();

        m_UsedColorsPos = 0;
        m_ColorMapping.clear();
        m_MaterialMapping.clear();
        m_ModelSceneTreeMapping.clear();
        m_HasEmission = false;
    }

    void CMagicaVoxelFormat::ParseFormat()
    {   
        std::string Signature(4, '\0');
        ReadData(&Signature[0], 4);
        Signature += "\0";

        // Checks the file header
        if(Signature != "VOX ")
            throw CVoxelLoaderException("Unknown file format");

        int Version = ReadData<int>();
        if(Version != 150)
            throw CVoxelLoaderException("Version: " + std::to_string(Version) + " is not supported");

        // First processes the materials that are at the end of the file. 
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        ProcessMaterialAndSceneGraph();
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        auto count = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
        count = 0;

        Skip(8);

        begin = std::chrono::steady_clock::now();
        if(!IsEof())
        {
            SChunkHeader Tmp = ReadData<SChunkHeader>();
            if(strncmp(Tmp.ID, "MAIN", sizeof(Tmp.ID)) == 0)
            {
                while (!IsEof())
                {
                    Tmp = ReadData<SChunkHeader>();

                    if(strncmp(Tmp.ID, "SIZE", sizeof(Tmp.ID)) == 0)
                    {
                        VoxelMesh m = ProcessSize();
                        Tmp = ReadData<SChunkHeader>();
                        if(strncmp(Tmp.ID, "XYZI", sizeof(Tmp.ID)) != 0)
                            throw CVoxelLoaderException("Can't understand the format.");

                        std::chrono::steady_clock::time_point begin1 = std::chrono::steady_clock::now();
                        ProcessXYZI(m);
                        std::chrono::steady_clock::time_point end1 = std::chrono::steady_clock::now();
                        auto count1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - begin1).count();

                        begin1 = std::chrono::steady_clock::now();
                        auto &voxels = m->GetVoxels();
                        voxels.generateVisibilityMask();
                        // auto visible = voxels.queryVisible();
                        end1 = std::chrono::steady_clock::now();
                        auto count2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - begin1).count();

                        int k = sizeof(Voxel);

                        m_Models.push_back(m);
                        auto halfSize = (m->GetSize() / 2.0);

                        auto treeNode = m_ModelSceneTreeMapping.at(m_Models.size() - 1);

                        // 1. Translation is always relative to the center of the voxel space size. (Without fraction)
                        // 2. All meshers centers the mehs in object space so we need to add the fractal part to the translation
                        // 3. Add the distance from object center to space center and add the start position of the voxel mesh

                        CVector spaceCenter = halfSize.Fract() + m->GetBBox().Beg + (m->GetBBox().GetSize() / 2 - halfSize);
                        treeNode->SetLocalOffset(spaceCenter);
                        std::swap(spaceCenter.y, spaceCenter.z);
                        spaceCenter.z *= -1;

                        auto pos = treeNode->GetPosition();
                        treeNode->SetPosition(pos + spaceCenter);

                        // TODO: Animation support.
                        treeNode->SetMesh(m); 
                        m->SetName(treeNode->GetName());
                        m->SetSceneNode(treeNode);                       
                    }
                    else if(strncmp(Tmp.ID, "RGBA", sizeof(Tmp.ID)) == 0)
                    {
                        for (size_t i = 0; i < m_ColorPalette.size(); i++)
                            ReadData((char*)m_ColorPalette[i].c, 4);
                    }
                    else
                        Skip(Tmp.ChunkContentSize + Tmp.ChildChunkSize);
                }
            }
        }
        end = std::chrono::steady_clock::now();
        count = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
        count = 0;

        auto texIT = m_Textures.find(TextureType::DIFFIUSE);
        if(texIT == m_Textures.end())
            m_Textures[TextureType::DIFFIUSE] = Texture(new CTexture(CVectori(m_ColorMapping.size(), 1, 0)));

        if(m_HasEmission)
        {
            auto texIT = m_Textures.find(TextureType::EMISSION);
            if(texIT == m_Textures.end())
                m_Textures[TextureType::EMISSION] = Texture(new CTexture(CVectori(m_ColorMapping.size(), 1, 0)));
        }

        // Creates the used color palette.
        for (auto &&c : m_ColorMapping)
        {
            m_Textures[TextureType::DIFFIUSE]->AddPixel(m_ColorPalette[c.first - 1], CVectori(c.second, 0, 0));
            if(m_HasEmission)
            {
                int MatIdx = 0;
                auto IT = m_MaterialMapping.find(c.first);
                if(IT != m_MaterialMapping.end())
                    MatIdx = IT->second;

                auto material = m_Materials[MatIdx];
                if(material->Power > 0)
                    m_Textures[TextureType::EMISSION]->AddPixel(m_ColorPalette[c.first - 1], CVectori(c.second, 0, 0));
            }
        }

        for (auto &&m : m_Models)
        {
            m->Colorpalettes() = m_Textures;
        }
    }

    void CMagicaVoxelFormat::LoadDefaultPalette()
    {
        m_ColorPalette.resize(256);
        for (size_t i = 0; i < m_ColorPalette.size(); i++)
        {
            memcpy(m_ColorPalette[i].c, &default_palette[i], 4);
        }
    }

    VoxelMesh CMagicaVoxelFormat::ProcessSize()
    {
        VoxelMesh Ret = VoxelMesh(new CVoxelMesh());

        CVectori Size;

        Size.x = ReadData<int>();
        Size.y = ReadData<int>();
        Size.z = ReadData<int>();

        Ret->SetSize(Size);

        return Ret;
    }

    void CMagicaVoxelFormat::ProcessXYZI(VoxelMesh m)
    {
        int VoxelCount = ReadData<int>();

        CVectori Beg(INT32_MAX, INT32_MAX, INT32_MAX), End;

        // Each model has it's used material attached, so we need to map the MagicaVoxel ID to the local one of the mesh.
        std::map<int, int> modelMaterialMapping;
        auto count1 = 0, count2 = 0;

        std::chrono::steady_clock::time_point begin1 = std::chrono::steady_clock::now();
        m->ReserveVoxels(VoxelCount);
        std::chrono::steady_clock::time_point end1 = std::chrono::steady_clock::now();
        size_t count3 = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - begin1).count();

        for (size_t i = 0; i < VoxelCount; i++)
        {
            CVectori vec;

            uint8_t data[4];

            begin1 = std::chrono::steady_clock::now();
            ReadData((char*)data, sizeof(data));
            end1 = std::chrono::steady_clock::now();
            count1 += std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - begin1).count();

            

            vec.x = data[0];
            vec.y = data[1];
            vec.z = data[2];
            int MatIdx = data[3];

            Beg = Beg.Min(vec);
            End = End.Max(vec);

            int Color = 0;
            bool Transparent = false;

            // Remaps the indices.
            auto IT = m_ColorMapping.find(MatIdx);
            if(IT == m_ColorMapping.end())
            {
                m_ColorMapping.insert({MatIdx, m_UsedColorsPos});
                Color = m_UsedColorsPos;
                m_UsedColorsPos++;
            }
            else
                Color = IT->second;

            int newMatIdx = 0;
            IT = m_MaterialMapping.find(MatIdx);
            if(IT != m_MaterialMapping.end())
            {
                newMatIdx = IT->second;
                Transparent = m_Materials[newMatIdx]->Transparency != 0.0;
                if(Transparent)
                {
                    int k= 0;
                    k++;
                }
            }

            // Remaps the material index to the local one.
            IT = modelMaterialMapping.find(MatIdx);
            if(IT != modelMaterialMapping.end())
                MatIdx = IT->second;
            else
            {
                m->Materials().push_back(m_Materials[newMatIdx]);
                modelMaterialMapping[MatIdx] = m->Materials().size() - 1;
                MatIdx = m->Materials().size() - 1;
            }

            begin1 = std::chrono::steady_clock::now();
            m->SetVoxel(vec, MatIdx, Color, Transparent);
            end1 = std::chrono::steady_clock::now();
            count2 += std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - begin1).count();
        } 

        m->SetBBox(CBBox(Beg, End));
    }

    void CMagicaVoxelFormat::ProcessMaterialAndSceneGraph()
    {
        std::map<int, Node> nodes;
        m_Materials.push_back(Material(new CMaterial()));

        if(!IsEof())
        {
            SChunkHeader Tmp = ReadData<SChunkHeader>();
            if(strncmp(Tmp.ID, "MAIN", sizeof(Tmp.ID)) == 0)
            {
                while (!IsEof())
                {
                    Tmp = ReadData<SChunkHeader>();

                    if(strncmp(Tmp.ID, "MATL", sizeof(Tmp.ID)) == 0)
                    {
                        Material Mat = Material(new CMaterial());
                        int ID = ReadData<int>();
                        int KeyValueCount = ReadData<int>();

                        std::string MaterialType;

                        for (int i = 0; i < KeyValueCount; i++)
                        {
                            int StrLen = ReadData<int>();

                            std::string Key(StrLen, '\0'); 
                            ReadData(&Key[0], StrLen);

                            if(Key == "_plastic")
                                continue;

                            StrLen = ReadData<int>();

                            std::string Value(StrLen, '\0'); 
                            ReadData(&Value[0], StrLen);

                            if(Key == "_type")
                                MaterialType = Value;
                            else if(Key == "_metal")
                                Mat->Metallic = std::stof(Value);
                            else if(Key == "_alpha")
                                Mat->Transparency = std::stof(Value);     
                            else if(Key == "_rough")
                                Mat->Roughness = std::stof(Value);
                            else if(Key == "_spec")
                                Mat->Specular = std::stof(Value);
                            else if(Key == "_ior")
                                Mat->IOR = std::stof(Value);
                            else if(Key == "_flux")
                            {
                                m_HasEmission = true;
                                Mat->Power = std::stof(Value);  
                            } 
                        }

                        if(MaterialType == "_diffuse" || MaterialType.empty())
                            continue;
                        
                        m_Materials.push_back(Mat);
                        m_MaterialMapping.insert({ID, m_Materials.size() - 1});
                    }
                    else if(strncmp(Tmp.ID, "nTRN", sizeof(Tmp.ID)) == 0)
                    {
                        auto tmp = ProcessTransformNode();
                        nodes.insert({tmp->NodeID, tmp});
                    }
                    else if(strncmp(Tmp.ID, "nGRP", sizeof(Tmp.ID)) == 0)
                    {
                        auto tmp = ProcessGroupNode();
                        nodes.insert({tmp->NodeID, tmp});
                    }
                    else if(strncmp(Tmp.ID, "nSHP", sizeof(Tmp.ID)) == 0)
                    {
                        auto tmp = ProcessShapeNode();
                        nodes.insert({tmp->NodeID, tmp});
                    }
                    else
                        Skip(Tmp.ChunkContentSize + Tmp.ChildChunkSize);
                }
            }
        }

        std::stack<int> nodeIDs;
        std::stack<SceneNode> sceneNodes;

        SceneNode currentNode = m_SceneTree;

        nodeIDs.push(0);
        while (!nodeIDs.empty())
        {
            Node tmp = nodes[nodeIDs.top()];

            switch (tmp->Type)
            {
                case NodeType::TRANSFORM:
                {
                    auto transform = std::static_pointer_cast<STransformNode>(tmp);
                    nodeIDs.pop();

                    currentNode->SetPosition(transform->Translation);
                    currentNode->SetRotation(transform->Rotation);
                    currentNode->SetName(transform->Name);

                    nodeIDs.push(transform->ChildID);
                } break;

                case NodeType::GROUP:
                {
                    auto group = std::static_pointer_cast<SGroupNode>(tmp);
                    if(group->ChildIdx > 0)
                    {
                        currentNode = sceneNodes.top();
                        sceneNodes.pop();
                    }

                    if(group->ChildIdx < group->ChildrensID.size())
                    {
                        sceneNodes.push(currentNode);
                        auto oldCurrent = currentNode;
                        currentNode = SceneNode(new CSceneNode());
                        oldCurrent->AddChild(currentNode);

                        nodeIDs.push(group->ChildrensID[group->ChildIdx]);
                        group->ChildIdx++;
                    }
                    else                     
                        nodeIDs.pop();
                } break;

                case NodeType::SHAPE:
                {
                    nodeIDs.pop();
                    auto shapes = std::static_pointer_cast<SShapeNode>(tmp);

                    for (auto &&m : shapes->Models)
                        m_ModelSceneTreeMapping.insert({m, currentNode});                
                } break;
            }
        }

        Reset();
    }

    CMagicaVoxelFormat::TransformNode CMagicaVoxelFormat::ProcessTransformNode()
    {
        TransformNode Ret = TransformNode(new STransformNode());

        Ret->NodeID = ReadData<int>();
        
        // Skips the dictionary
        int keys = ReadData<int>();
        for (size_t i = 0; i < keys; i++)
        {
            int size = ReadData<int>();
            std::string key(size, '\0'); 
            ReadData(&key[0], size);
            if(key == "_name")
            {              
                size = ReadData<int>();
                std::string value(size, '\0'); 
                ReadData(&value[0], size);
                Ret->Name = value;
            }
            else
            {
                int size = ReadData<int>();
                Skip(size);
            }            
        }

        Ret->ChildID = ReadData<int>();
        Skip(sizeof(int));
        Ret->LayerID = ReadData<int>();

        int frames = ReadData<int>();
        for (size_t i = 0; i < frames; i++)
        {
            keys = ReadData<int>();
            for (size_t j = 0; j < keys; j++)
            {
                int size = ReadData<int>();
                std::string Key(size, '\0'); 
                ReadData(&Key[0], size);

                if(Key == "_t")
                {
                    size = ReadData<int>();
                    std::string Value(size, '\0'); 
                    ReadData(&Value[0], size);

                    std::stringstream tmp;
                    tmp << Value;

                    // Scenetree always in OpenGL Y-UP Space
                    tmp >> Ret->Translation.x >> Ret->Translation.z >> Ret->Translation.y;
                }
                else if(Key == "_r")
                {
                    size = ReadData<int>();
                    std::string Value(size, '\0'); 
                    ReadData(&Value[0], size);

                    char rot = std::stoi(Value);

                    char idx1 = rot & 3;
                    char idx2 = (rot >> 2) & 3;
                    char idx3 = 3 - idx1 - idx2;

                    auto rotation = CMat4x4(CVector4(0, 0, 0, 0),
                                            CVector4(0, 0, 0, 0),
                                            CVector4(0, 0, 0, 0),
                                            CVector4(0, 0, 0, 1));

                    rotation.x.v[idx1] = ((rot & 0x10) == 0x10) ? -1 : 1;
                    rotation.y.v[idx2] = ((rot & 0x20) == 0x20) ? -1 : 1;
                    rotation.z.v[idx3] = ((rot & 0x40) == 0x40) ? -1 : 1;

                    // Gets the euler angle, y is the up axis.
                    Ret->Rotation = rotation.GetEuler();
                    std::swap(Ret->Rotation.y, Ret->Rotation.z);
                }
                else if(Key == "_f")
                    Skip(ReadData<int>());
            }
        }

        return Ret;
    }
    
    CMagicaVoxelFormat::GroupNode CMagicaVoxelFormat::ProcessGroupNode()
    {
        GroupNode Ret = GroupNode(new SGroupNode());

        Ret->NodeID = ReadData<int>();
        SkipDict();

        int childs = ReadData<int>();
        for (size_t i = 0; i < childs; i++)
            Ret->ChildrensID.push_back(ReadData<int>());

        return Ret;
    }

    CMagicaVoxelFormat::ShapeNode CMagicaVoxelFormat::ProcessShapeNode()
    {
        ShapeNode Ret = ShapeNode(new SShapeNode());

        Ret->NodeID = ReadData<int>();
        SkipDict();

        int childs = ReadData<int>();
        for (size_t i = 0; i < childs; i++)
        {
            Ret->Models.push_back(ReadData<int>());

            // Skips the dictionary
            int keys = ReadData<int>();
            for (size_t i = 0; i < keys; i++)
            {
                Skip(ReadData<int>());
                Skip(ReadData<int>());
            }
        }

        return Ret;
    }

    void CMagicaVoxelFormat::SkipDict()
    {
        int keys = ReadData<int>();
        for (size_t i = 0; i < keys; i++)
        {
            int size = ReadData<int>();
            Skip(size);
            size = ReadData<int>();
            Skip(size);
        }
    }
} // namespace VoxelOptimizer
