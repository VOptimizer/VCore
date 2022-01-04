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

#include "../../FileUtils.hpp"
#include "VEditFormat.hpp"
#include <stb_image.h>
#include <VoxelOptimizer/Exceptions.hpp>

namespace VoxelOptimizer
{
    void CFileHeader::Serialize(CBinaryStream &strm)
    {
        strm.write(m_Signature, 5);
        strm << m_Version;
        strm.write(ProgrammVersion, 23);
    }

    void ISection::Serialize(CBinaryStream &strm)
    {
        strm << m_Type;
    }

    void ISection::Deserialize(CBinaryStream &strm)
    {

    }

    void ISection::SkipAnyType(CBinaryStream &strm, AnyType type)
    {
        switch (type)
        {
            case AnyType::STRING:
            {
                std::string tmp;
                strm >> tmp;
            }break;
        
            case AnyType::FLOAT:
            case AnyType::INT32:
            case AnyType::UINT32:
            {
                strm.skip(sizeof(int));
            }break;

            default:
                throw CVoxelLoaderException("Invalid type!");
        }
    }

    void CMetaSection::Serialize(CBinaryStream &strm)
    {
        ISection::Serialize(strm);
        uint32_t size = sizeof(uint32_t) * 4 + Author.size() + Company.size() + Copyright.size() + Name.size();
        strm << size;
        strm << Author;
        strm << Company;
        strm << Copyright;
        strm << Name;
    }

    void CMetaSection::Deserialize(CBinaryStream &strm)
    {
        uint32_t size;
        strm >> size;

        auto startOff = strm.offset();

        strm >> Author;
        strm >> Company;
        strm >> Copyright;
        strm >> Name;

        //Skips extra data, if there is any.
        if(strm.offset() - startOff < size)
            strm.skip(size - (strm.offset() - startOff));
    }

    void CMaterialSection::Serialize(CBinaryStream &strm)
    {
        ISection::Serialize(strm);

        CBinaryStream dict;
        dict << (uint32_t)7; //Key value pairs

        dict << std::string("name");
        dict << AnyType::STRING;
        dict << Name;

        dict << std::string("metallic");
        dict << AnyType::FLOAT;
        dict << Mat->Metallic;

        dict << std::string("specular");
        dict << AnyType::FLOAT;
        dict << Mat->Specular;

        dict << std::string("roughness");
        dict << AnyType::FLOAT;
        dict << Mat->Roughness;

        dict << std::string("ior");
        dict << AnyType::FLOAT;
        dict << Mat->IOR;

        dict << std::string("power");
        dict << AnyType::FLOAT;
        dict << Mat->Power;

        dict << std::string("transparency");
        dict << AnyType::FLOAT;
        dict << Mat->Transparency;

        auto dictData = dict.data();
        strm << (uint32_t)dictData.size();
        strm.write(dictData.data(), dictData.size());
    }

    void CMaterialSection::Deserialize(CBinaryStream &strm)
    {
        Mat = Material(new CMaterial());

        uint32_t size;
        strm >> size;

        auto startOff = strm.offset();
        uint32_t dictSize;
        strm >> dictSize;

        for (size_t i = 0; i < dictSize; i++)
        {
            std::string key;
            strm >> key;

            AnyType type;
            strm >> type;

            switch (Adler32(key.c_str()))
            {
                case Adler32("name"):
                {
                    if(type != AnyType::STRING)
                        SkipAnyType(strm, type);
                    else
                        strm >> Name;
                }break;

                case Adler32("metallic"):
                {
                    if(type != AnyType::FLOAT)
                        SkipAnyType(strm, type);
                    else
                        strm >> Mat->Metallic;
                }break;
            
                case Adler32("specular"):
                {
                    if(type != AnyType::FLOAT)
                        SkipAnyType(strm, type);
                    else
                        strm >> Mat->Specular;
                }break;

                case Adler32("roughness"):
                {
                    if(type != AnyType::FLOAT)
                        SkipAnyType(strm, type);
                    else
                        strm >> Mat->Roughness;
                }break;
            
                case Adler32("ior"):
                {
                    if(type != AnyType::FLOAT)
                        SkipAnyType(strm, type);
                    else
                        strm >> Mat->IOR;
                }break;

                case Adler32("power"):
                {
                    if(type != AnyType::FLOAT)
                        SkipAnyType(strm, type);
                    else
                        strm >> Mat->Power;
                }break;
            
                case Adler32("transparency"):
                {
                    if(type != AnyType::FLOAT)
                        SkipAnyType(strm, type);
                    else
                        strm >> Mat->Transparency;
                }break;
            }
        }

        //Skips extra data, if there is any.
        if(strm.offset() - startOff < size)
            strm.skip(size - (strm.offset() - startOff));
    }

    void CColorpaletteSection::Serialize(CBinaryStream &strm)
    {
        ISection::Serialize(strm);
        strm << (uint32_t)(Colors->Size().x * Colors->Size().y * 4);
        for (auto &&p : Colors->Pixels())
        {
            CColor c;
            c.FromRGBA(p);

            strm.write((char*)c.c, sizeof(c.c));
        }
    }

    void CColorpaletteSection::Deserialize(CBinaryStream &strm)
    {
        uint32_t size;
        strm >> size;

        Colors = Texture(new CTexture());
        for (size_t i = 0; i < size; i += 4)
        {
            CColor c;
            strm.read((char*)c.c, sizeof(c.c));
            Colors->AddPixel(c);
        }
    }

    void CVoxelSection::Serialize(CBinaryStream &strm)
    {
        ISection::Serialize(strm);
        std::vector<char> thumbnail;
        
        if(Thumbnail)
            thumbnail = Thumbnail->AsPNG();

        uint32_t size = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(CVector) + sizeof(uint32_t) + thumbnail.size() + Mesh->GetVoxels().size() * (sizeof(CVector) + sizeof(uint32_t) * 4);
        strm << size;
        strm << (uint32_t)0; //Properties
        strm << (uint32_t)0; //Name
        strm << (uint32_t)thumbnail.size();
        strm.write(thumbnail.data(), thumbnail.size());
        strm.write((char*)Mesh->GetSize().v, sizeof(float) * 3);
        strm << (uint32_t)Mesh->GetVoxels().size();

        for (auto &&v : Mesh->GetVoxels())
        {
            strm.write((char*)v.first.v, sizeof(float) * 3);

            auto mat = Mesh->Materials()[v.second->Material];

            strm << m_MaterialMapping.at(mat);
            strm << v.second->Color;
            strm << (uint32_t)0; // Type
            strm << (uint32_t)0; // Properties
        }
    }

    void CVoxelSection::Deserialize(CBinaryStream &strm)
    {
        uint32_t size;
        strm >> size;
        auto startOff = strm.offset();

        strm.skip(sizeof(uint32_t) * 2); // Properties + Name
        uint32_t thumbSize;
        strm >> thumbSize;

        if(thumbSize > 0)
        {
            std::vector<char> img(thumbSize, 0);
            strm.read(&img[0], thumbSize);

            int w, h, c;
            uint32_t *ImgData = (uint32_t*)stbi_load_from_memory((unsigned char*)img.data(), thumbSize, &w, &h, &c, 4);

            Thumbnail = Texture(new CTexture(CVector(w, h, 0), ImgData));
            free(ImgData);
        }

        Mesh = VoxelMesh(new CVoxelMesh());
        CVector voxlespaceSize;
        strm >> voxlespaceSize;
        Mesh->SetSize(voxlespaceSize);

        std::map<int, int> modelMaterialMapping;

        uint32_t voxels;
        strm >> voxels;
        for (size_t i = 0; i < voxels; i++)
        {
            CVector pos;
            uint32_t matIdx, colorIdx;

            strm >> pos;
            strm >> matIdx;
            strm >> colorIdx;
            strm.skip(sizeof(uint32_t) * 2); // Type + Properties

            auto mat = m_Materials[matIdx];

            auto it = modelMaterialMapping.find(matIdx);
            if(it != modelMaterialMapping.end())
                matIdx = it->second;
            else
            {
                Mesh->Materials().push_back(mat);

                modelMaterialMapping[matIdx] = Mesh->Materials().size() - 1;
                matIdx = modelMaterialMapping[matIdx];
            }

            Mesh->SetVoxel(pos, matIdx, colorIdx, mat->Transparency != 0.0);
        }
        
        //Skips extra data, if there is any.
        if(strm.offset() - startOff < size)
            strm.skip(size - (strm.offset() - startOff));
    }

    void CSceneTreeSection::Serialize(CBinaryStream &strm)
    {
        ISection::Serialize(strm);
        CBinaryStream tree;
        SerializeTree(Tree, tree);
        
        auto treeData = tree.data();
        strm << (uint32_t)treeData.size();
        strm.write(treeData.data(), treeData.size());
    }

    void CSceneTreeSection::SerializeTree(SceneNode tree, CBinaryStream &strm)
    {
        strm << tree->GetName();
        strm << tree->GetPosition();
        strm << tree->GetRotation();
        strm << tree->GetScale();

        uint32_t meshID = -1;
        auto mesh = tree->GetMesh();
        if(mesh)
        {
            auto it = std::find(m_Meshes.begin(), m_Meshes.end(), mesh);
            if(it != m_Meshes.end())
                meshID = it - m_Meshes.begin();
        }

        strm << meshID;
        strm << tree->ChildCount();

        for (auto &&c : *tree)
            SerializeTree(c, strm);
    }

    SceneNode CSceneTreeSection::DeserializeTree(CBinaryStream &strm)
    {
        SceneNode ret = SceneNode(new CSceneNode());

        std::string name;
        CVector pos, rot, scale;

        strm >> name;
        strm >> pos;
        strm >> rot;
        strm >> scale;

        ret->SetName(name);
        ret->SetPosition(pos);
        ret->SetRotation(rot);
        ret->SetScale(scale);

        uint32_t meshID;
        strm >> meshID;

        if(meshID != -1)
            ret->SetMesh(m_Meshes[meshID]);

        uint32_t childs;
        strm >> childs;
        for (size_t i = 0; i < childs; i++)
            ret->AddChild(DeserializeTree(strm));        

        return ret;
    }

    void CSceneTreeSection::Deserialize(CBinaryStream &strm)
    {
        uint32_t size;
        strm >> size;
        auto startOff = strm.offset();

        Tree = DeserializeTree(strm);
        
        //Skips extra data, if there is any.
        if(strm.offset() - startOff < size)
            strm.skip(size - (strm.offset() - startOff));
    }

    std::vector<char> CVEditFormat::Save(const std::vector<VoxelMesh> &meshes)
    {
        CBinaryStream stream;
        CFileHeader header;
        header.Serialize(stream);

        int materialIdxCounter = 0;
        std::map<Material, int> materials;

        for (auto &&m : meshes)
        {
            for (auto &&mat : m->Materials())
            {
                auto it = materials.find(mat);
                if(it == materials.end())
                    materials[mat] = materialIdxCounter++;
            }
        }

        for (auto &&m : materials)
        {
            CMaterialSection mat;
            mat.Mat = m.first;

            mat.Serialize(stream);
        }

        CColorpaletteSection colors;
        colors.Colors = meshes.front()->Colorpalettes()[TextureType::DIFFIUSE];
        colors.Serialize(stream);

        for (auto &&m : meshes)
        {
            CVoxelSection voxel(materials);
            voxel.Mesh = m;

            voxel.Serialize(stream);
        }

        CSceneTreeSection sceneTree(meshes);
        sceneTree.Tree = m_SceneTree;
        sceneTree.Serialize(stream);

        return stream.data();
    }

    void CVEditFormat::ParseFormat()
    {
        std::string Signature(5, '\0');
        ReadData(&Signature[0], 5);
        Signature += "\0";

        // Checks the file header
        if(Signature != "VEDIT")
            throw CVoxelLoaderException("Unknown file format");

        int32_t version = ReadData<int32_t>();
        if(version != 0x1)
            throw CVoxelLoaderException("Version: " + std::to_string(version) + " is not supported");

        Skip(23); // Programm version.

        while(!IsEof())
        {
            int32_t type = ReadData<int32_t>();
            switch(type)
            {
                case SectionType::MATERIAL:
                {
                    CMaterialSection mat;
                    mat.Deserialize(m_DataStream);
                    m_Materials.push_back(mat.Mat);
                }break;

                case SectionType::COLORPALETTE:
                {
                    CColorpaletteSection colors;
                    colors.Deserialize(m_DataStream);

                    m_Textures[TextureType::DIFFIUSE] = colors.Colors;
                }break;

                case SectionType::VOXELS:
                {
                    CVoxelSection voxel(m_Materials);
                    voxel.Deserialize(m_DataStream);

                    m_Models.push_back(voxel.Mesh);
                    voxel.Mesh->Colorpalettes() = m_Textures;
                    voxel.Mesh->RecalcBBox();
                }break;

                case SectionType::SCENE_TREE:
                {
                    CSceneTreeSection sceneTree(m_Models);
                    sceneTree.Deserialize(m_DataStream);

                    m_SceneTree = sceneTree.Tree;
                }break;

                default:
                {
                    uint32_t size = ReadData<uint32_t>();
                    Skip(size);
                }break;
            };
        }
    }
} // namespace VoxelOptimizer
