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
#include <VoxelOptimizer/Formats/VEditFormat.hpp>
#include <stb_image.h>
#include <stb_image_write.h>
#include <VoxelOptimizer/Misc/Exceptions.hpp>

namespace VoxelOptimizer
{
    enum SectionType
    {
        META,
        MATERIAL,
        COLORPALETTE,
        VOXELS,
        SCENE_TREE,
        TEXTURE_PLANES
    };

    enum AnyType : uint8_t
    {
        STRING,
        FLOAT,
        INT32,
        UINT32,
        VECTOR3I
    };

    inline std::ostream &operator<<(std::ostream &_strm, const AnyType &_val)
    {
        _strm << (uint8_t)_val;
        return _strm;
    }

    inline std::istream &operator>>(std::istream &_strm, AnyType &_val)
    {
        uint8_t val;
        _strm >> val;
        _val = (AnyType)val;
        return _strm;
    }

    class CFileHeader
    {
        public:
            CFileHeader() 
            {
                strncpy(m_Signature, "VEDIT", 5);
                m_Version = 0x1;
                memset(ProgrammVersion, 0, sizeof(ProgrammVersion));
            }

            void Serialize(CBinaryStream &strm);

        private:
            char m_Signature[5];
            int32_t m_Version;
            char ProgrammVersion[23];
    };

    class ISection
    {
        public:
            ISection(int32_t type) : m_Type(type) {}

            virtual void Serialize(CBinaryStream &strm);
            virtual void Deserialize(CBinaryStream &strm);

        protected:
            void SkipAnyType(CBinaryStream &strm, AnyType type);
            char *CompressStream(CBinaryStream &strm, int &dataSize);
            CBinaryStream DecompressStream(CBinaryStream &strm, int dataSize);

            int32_t m_Type;
    };

    class CMetaSection : public ISection
    {
        public:
            CMetaSection() : ISection(SectionType::META) {}

            virtual void Serialize(CBinaryStream &strm) override;
            virtual void Deserialize(CBinaryStream &strm) override;

            std::string Author;
            std::string Company;
            std::string Copyright;
            std::string Name;
    };

    class CMaterialSection : public ISection
    {
        public:
            CMaterialSection() : ISection(SectionType::MATERIAL) {}

            virtual void Serialize(CBinaryStream &strm) override;
            virtual void Deserialize(CBinaryStream &strm) override;

            std::string Name;
            Material Mat;
    };

    class CColorpaletteSection : public ISection
    {
        public:
            CColorpaletteSection() : ISection(SectionType::COLORPALETTE) {}

            virtual void Serialize(CBinaryStream &strm) override;
            virtual void Deserialize(CBinaryStream &strm) override;

            Texture Colors;
    };

    class CVoxelSection : public ISection
    {
        public:
            CVoxelSection(const std::vector<Material> &materials) : ISection(SectionType::VOXELS), m_MaterialMapping(std::map<Material, int>()), m_Materials(materials) {}
            CVoxelSection(const std::map<Material, int> &materialMapping) : ISection(SectionType::VOXELS), m_MaterialMapping(materialMapping), m_Materials(std::vector<Material>()) {}

            virtual void Serialize(CBinaryStream &strm) override;
            virtual void Deserialize(CBinaryStream &strm) override;

            VoxelMesh Mesh;

        private:
            const std::map<Material, int> &m_MaterialMapping;
            const std::vector<Material> &m_Materials;
    };

    class CSceneTreeSection : public ISection
    {
        public:
            CSceneTreeSection(const std::vector<VoxelMesh> &meshes) : ISection(SectionType::SCENE_TREE), m_Meshes(meshes) {}

            virtual void Serialize(CBinaryStream &strm) override;
            virtual void Deserialize(CBinaryStream &strm) override;

            SceneNode Tree;

        private:
            void SerializeTree(SceneNode tree, CBinaryStream &strm);
            SceneNode DeserializeTree(CBinaryStream &strm);

            const std::vector<VoxelMesh> &m_Meshes;
    };

    class CTexturePlanes : public ISection
    {
        public:
            CTexturePlanes() : ISection(SectionType::TEXTURE_PLANES) {}

            std::string Name;
            Texture Planes;
            CVectori VoxelSpaceSize;
            SPlanesInfo PlanesInfo;

            virtual void Serialize(CBinaryStream &strm) override;
            virtual void Deserialize(CBinaryStream &strm) override;
    };

    //////////////////////////////////////////////////
    // CFileHeader functions
    //////////////////////////////////////////////////

    void CFileHeader::Serialize(CBinaryStream &strm)
    {
        strm.write(m_Signature, 5);
        strm << m_Version;
        strm.write(ProgrammVersion, 23);
    }

    //////////////////////////////////////////////////
    // ISection functions
    //////////////////////////////////////////////////

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

    char *ISection::CompressStream(CBinaryStream &strm, int &dataSize)
    {
        auto data = strm.data();
        return (char*)stbi_zlib_compress((uint8_t*)data.data(), data.size(), &dataSize, 6);
    }

    CBinaryStream ISection::DecompressStream(CBinaryStream &strm, int dataSize)
    {
        CBinaryStream ret;

        std::vector<char> data(dataSize, 0);
        strm.read(&data[0], dataSize);
        int decompressSize = 0;

        char *decompressedData = stbi_zlib_decode_malloc(data.data(), dataSize, &decompressSize);
        ret.write(decompressedData, decompressSize);
        free(decompressedData);

        ret.reset();
        return ret;
    }

    //////////////////////////////////////////////////
    // CMetaSection functions
    //////////////////////////////////////////////////

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

    //////////////////////////////////////////////////
    // CMaterialSection functions
    //////////////////////////////////////////////////

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

    //////////////////////////////////////////////////
    // CColorpaletteSection functions
    //////////////////////////////////////////////////

    void CColorpaletteSection::Serialize(CBinaryStream &strm)
    {
        ISection::Serialize(strm);

        // Array of colors
        CBinaryStream colors;
        colors << (uint32_t)Colors->Pixels().size();
        for (auto &&p : Colors->Pixels())
        {
            CColor c;
            c.FromRGBA(p);

            colors.write((char*)c.c, sizeof(c.c));
        }

        // Compresses the color palette.
        int dataSize;
        auto data = CompressStream(colors, dataSize);

        // Calculates the size of this section.
        uint32_t size = sizeof(uint32_t) * 2 + (uint32_t)dataSize;

        // Write section.
        strm << size;
        strm << (uint32_t)0;    // Name
        strm << (uint32_t)dataSize;
        strm.write(data, dataSize);  

        free(data);
    }

    void CColorpaletteSection::Deserialize(CBinaryStream &strm)
    {
        uint32_t size;
        strm >> size;

        strm.skip(sizeof(uint32_t)); // Name
        uint32_t datasize;
        strm >> datasize;

        auto data = CBinaryStream();//DecompressStream(strm, datasize);

        Colors = Texture(new CTexture());
        uint32_t arraySize;
        data >> arraySize;

        for (size_t i = 0; i < arraySize; i++)
        {
            CColor c;
            data.read((char*)c.c, sizeof(c.c));
            Colors->AddPixel(c);
        }
    }

    //////////////////////////////////////////////////
    // CVoxelSection functions
    //////////////////////////////////////////////////

    void CVoxelSection::Serialize(CBinaryStream &strm)
    {
        ISection::Serialize(strm);
        std::vector<char> thumbnail;
        
        if(Mesh->GetThumbnail())
            thumbnail = Mesh->GetThumbnail()->AsPNG();

        CBinaryStream voxels;

        // Array of voxel data
        // Array size
        voxels << (uint32_t)Mesh->GetVoxels().size();
        for (auto &&v : Mesh->GetVoxels())
        {
            CVectori pos(v.first.x, v.first.z, v.first.y);
            voxels << pos;  // Position

            auto mat = Mesh->Materials()[v.second->Material];

            voxels << m_MaterialMapping.at(mat);        // Gets the material index
            voxels << v.second->Color;                  // Color index
            voxels << (uint8_t)v.second->VisibilityMask;// Visibility mask
            voxels << (uint32_t)0;                      // Type. Reserved for future
            voxels << (uint32_t)0;                      // Properties. Reserved for future.
        }

        int dataSize = 0;
        char *data = CompressStream(voxels, dataSize);

        uint32_t size = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(CVectori) + sizeof(bool) + sizeof(uint32_t) + thumbnail.size() + sizeof(uint32_t) + dataSize;
        strm << size;
        strm << (uint32_t)0;                                        // Properties
        strm << Mesh->GetName();                                    // Name
        strm << (uint32_t)thumbnail.size();
        strm.write(thumbnail.data(), thumbnail.size());
        strm << (uint32_t)0;                                        // Colorpalette id. Reserved for the future.
        strm << Mesh->GetPivot();                                   // Pivot

        CVectori spaceSize(Mesh->GetSize().x, Mesh->GetSize().z, Mesh->GetSize().y);

        strm << spaceSize;                                               // Voxel space size.

        strm << dataSize;                                           // Compressed voxel data size
        strm.write(data, dataSize);                                 // Compressed voxel data
        free(data);
    }

    void CVoxelSection::Deserialize(CBinaryStream &strm)
    {
        uint32_t size;
        strm >> size;
        auto startOff = strm.offset();

        strm.skip(sizeof(uint32_t)); // Properties
        std::string name;
        strm >> name;   // Name

        uint32_t thumbSize;
        strm >> thumbSize;

        Mesh = VoxelMesh(new CVoxelMesh());
        Mesh->SetName(name);
        if(thumbSize > 0)
        {
            std::vector<char> img(thumbSize, 0);
            strm.read(&img[0], thumbSize);

            int w, h, c;
            uint32_t *ImgData = (uint32_t*)stbi_load_from_memory((unsigned char*)img.data(), thumbSize, &w, &h, &c, 4);
            if(ImgData)
            {
                Mesh->SetThumbnail(Texture(new CTexture(CVector(w, h, 0), ImgData)));
                free(ImgData);
            }
        }
        strm.skip(sizeof(uint32_t)); // Colorpalette id. Reserved for the future.

        CVector pivot;
        strm >> pivot;
        Mesh->SetPivot(pivot);

        CVectori voxlespaceSize;
        strm >> voxlespaceSize;
        std::swap(voxlespaceSize.y, voxlespaceSize.z);
        Mesh->SetSize(voxlespaceSize);

        std::map<int, int> modelMaterialMapping;

        uint32_t compressedDataSize = 0;
        strm >> compressedDataSize;
        CBinaryStream voxelStrm = CBinaryStream();//DecompressStream(strm, compressedDataSize);

        uint32_t voxels;
        voxelStrm >> voxels;
        for (size_t i = 0; i < voxels; i++)
        {
            CVectori pos;
            uint32_t matIdx, colorIdx;
            uint8_t mask;

            voxelStrm >> pos;
            voxelStrm >> matIdx;
            voxelStrm >> colorIdx;
            voxelStrm >> mask;
            voxelStrm.skip(sizeof(uint32_t) * 2); // Type + Properties

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

            std::swap(pos.y, pos.z);
            Mesh->SetVoxel(pos, matIdx, colorIdx, mat->Transparency != 0.0, (CVoxel::Visibility)mask);
        }
        
        //Skips extra data, if there is any.
        if(strm.offset() - startOff < size)
            strm.skip(size - (strm.offset() - startOff));
    }

    //////////////////////////////////////////////////
    // CSceneTreeSection functions
    //////////////////////////////////////////////////

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

    //////////////////////////////////////////////////
    // CTexturePlanes functions
    //////////////////////////////////////////////////

    void CTexturePlanes::Serialize(CBinaryStream &strm)
    {
        ISection::Serialize(strm);

        CBinaryStream data;
        data << Name;
        data << VoxelSpaceSize;

        auto texture = Planes->AsPNG();
        data << (uint32_t)texture.size();
        data.write(texture.data(), texture.size());

        CVectori topsize = PlanesInfo.Top.GetSize() - CVector(1, 1, 1);
        CVectori frontsize = PlanesInfo.Front.GetSize() - CVector(1, 1, 1);
        CVectori leftsize = PlanesInfo.Left.GetSize() - CVector(1, 1, 1);
        CVectori rightsize = PlanesInfo.Right.GetSize() - CVector(1, 1, 1);
        CVectori bottomsize = PlanesInfo.Bottom.GetSize() - CVector(1, 1, 1);
        CVectori backsize = PlanesInfo.Back.GetSize() - CVector(1, 1, 1);

        uint32_t pairs = 0;

        CBinaryStream dict;
        if(!topsize.IsZero())
        {
            dict << std::string("POS_T");
            dict << AnyType::VECTOR3I;
            dict << PlanesInfo.Top.Beg;
            pairs++;

            dict << std::string("SIZE_T");
            dict << AnyType::VECTOR3I;
            dict << topsize;
            pairs++;
        }

        if(!frontsize.IsZero())
        {
            dict << std::string("POS_F");
            dict << AnyType::VECTOR3I;
            dict << PlanesInfo.Front.Beg;
            pairs++;

            dict << std::string("SIZE_F");
            dict << AnyType::VECTOR3I;
            dict << frontsize;
            pairs++;
        }

        if(!leftsize.IsZero())
        {
            dict << std::string("POS_L");
            dict << AnyType::VECTOR3I;
            dict << PlanesInfo.Left.Beg;
            pairs++;

            dict << std::string("SIZE_L");
            dict << AnyType::VECTOR3I;
            dict << leftsize;
            pairs++;
        }

        if(!bottomsize.IsZero())
        {
            dict << std::string("POS_B");
            dict << AnyType::VECTOR3I;
            dict << PlanesInfo.Bottom.Beg;
            pairs++;

            dict << std::string("SIZE_B");
            dict << AnyType::VECTOR3I;
            dict << bottomsize;
            pairs++;
        }

        if(!rightsize.IsZero())
        {
            dict << std::string("POS_R");
            dict << AnyType::VECTOR3I;
            dict << PlanesInfo.Right.Beg;
            pairs++;

            dict << std::string("SIZE_R");
            dict << AnyType::VECTOR3I;
            dict << rightsize;
            pairs++;
        }

        if(!backsize.IsZero())
        {
            dict << std::string("POS_BA");
            dict << AnyType::VECTOR3I;
            dict << PlanesInfo.Back.Beg;
            pairs++;

            dict << std::string("SIZE_BA");
            dict << AnyType::VECTOR3I;
            dict << backsize;
            pairs++;
        }

        data << pairs;
        auto dictData = dict.data();
        data.write(dictData.data(), dictData.size());

        strm << (uint32_t)data.size();
        auto dataData = data.data();
        strm.write(dataData.data(), dataData.size());
    }

    void CTexturePlanes::Deserialize(CBinaryStream &strm)
    {
        uint32_t size;
        strm >> size;
        auto startOff = strm.offset();

        strm >> Name;
        strm >> VoxelSpaceSize;

        uint32_t planesSize;
        strm >> planesSize;
        
        if(planesSize > 0)
        {
            std::vector<char> img(planesSize, 0);
            strm.read(&img[0], planesSize);

            int w, h, c;
            uint32_t *ImgData = (uint32_t*)stbi_load_from_memory((unsigned char*)img.data(), planesSize, &w, &h, &c, 4);
            if(ImgData)
            {
                Planes = std::make_shared<CTexture>(CVector(w, h, 0), ImgData);
                free(ImgData);
            }

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
                    case Adler32("POS_T"):
                    {
                        if(type != AnyType::VECTOR3I)
                            SkipAnyType(strm, type);
                        else
                            strm >> PlanesInfo.Top.Beg;
                    }break;

                    case Adler32("SIZE_T"):
                    {
                        if(type != AnyType::VECTOR3I)
                            SkipAnyType(strm, type);
                        else
                        {
                            strm >> PlanesInfo.Top.End;
                            PlanesInfo.Top.End += PlanesInfo.Top.Beg + CVectori(1, 1, 1);
                        }
                    }break;

                    case Adler32("POS_F"):
                    {
                        if(type != AnyType::VECTOR3I)
                            SkipAnyType(strm, type);
                        else
                            strm >> PlanesInfo.Front.Beg;
                    }break;

                    case Adler32("SIZE_F"):
                    {
                        if(type != AnyType::VECTOR3I)
                            SkipAnyType(strm, type);
                        else
                        {
                            strm >> PlanesInfo.Front.End;
                            PlanesInfo.Front.End += PlanesInfo.Front.Beg + CVectori(1, 1, 1);
                        }
                    }break;

                    case Adler32("POS_L"):
                    {
                        if(type != AnyType::VECTOR3I)
                            SkipAnyType(strm, type);
                        else
                            strm >> PlanesInfo.Left.Beg;
                    }break;

                    case Adler32("SIZE_L"):
                    {
                        if(type != AnyType::VECTOR3I)
                            SkipAnyType(strm, type);
                        else
                        {
                            strm >> PlanesInfo.Left.End;
                            PlanesInfo.Left.End += PlanesInfo.Left.Beg + CVectori(1, 1, 1);
                        }
                    }break;

                    case Adler32("POS_B"):
                    {
                        if(type != AnyType::VECTOR3I)
                            SkipAnyType(strm, type);
                        else
                            strm >> PlanesInfo.Bottom.Beg;
                    }break;

                    case Adler32("SIZE_B"):
                    {
                        if(type != AnyType::VECTOR3I)
                            SkipAnyType(strm, type);
                        else
                        {
                            strm >> PlanesInfo.Bottom.End;
                            PlanesInfo.Bottom.End += PlanesInfo.Bottom.Beg + CVectori(1, 1, 1);
                        }
                    }break;

                    case Adler32("POS_R"):
                    {
                        if(type != AnyType::VECTOR3I)
                            SkipAnyType(strm, type);
                        else
                            strm >> PlanesInfo.Right.Beg;
                    }break;

                    case Adler32("SIZE_R"):
                    {
                        if(type != AnyType::VECTOR3I)
                            SkipAnyType(strm, type);
                        else
                        {
                            strm >> PlanesInfo.Right.End;
                            PlanesInfo.Right.End += PlanesInfo.Right.Beg + CVectori(1, 1, 1);
                        }
                    }break;

                    case Adler32("POS_BA"):
                    {
                        if(type != AnyType::VECTOR3I)
                            SkipAnyType(strm, type);
                        else
                            strm >> PlanesInfo.Back.Beg;
                    }break;

                    case Adler32("SIZE_BA"):
                    {
                        if(type != AnyType::VECTOR3I)
                            SkipAnyType(strm, type);
                        else
                        {
                            strm >> PlanesInfo.Back.End;
                            PlanesInfo.Back.End += PlanesInfo.Back.Beg + CVectori(1, 1, 1);
                        }
                    }break;
                }
            }
        }

        //Skips extra data, if there is any.
        if(strm.offset() - startOff < size)
            strm.skip(size - (strm.offset() - startOff));
    }

    //////////////////////////////////////////////////
    // CVEditFormat functions
    //////////////////////////////////////////////////

    std::vector<char> CVEditFormat::Save(const std::vector<VoxelMesh> &meshes)
    {
        m_Materials.clear();

        CBinaryStream stream;
        CFileHeader header;
        header.Serialize(stream);

        int materialIdxCounter = 0;
        std::map<Material, int> materials;

        // Creates an index map where each material is given its own unique index.
        for (auto &&m : meshes)
        {
            for (auto &&mat : m->Materials())
            {
                auto it = materials.find(mat);
                if(it == materials.end())
                {
                    m_Materials.push_back(mat);
                    materials[mat] = materialIdxCounter++;  // Generates a new index for a newly found material.
                }
            }
        }

        // Writes all materials to the file.
        for (auto &&m : m_Materials)
        {
            CMaterialSection mat;
            mat.Mat = m;

            mat.Serialize(stream);
        }
        m_Materials.clear();

        if(!meshes.empty())
        {
            // Writes the color palette to the file.
            CColorpaletteSection colors;
            colors.Colors = meshes.front()->Colorpalettes()[TextureType::DIFFIUSE];
            colors.Serialize(stream);

            // Writes all models to the file.
            for (auto &&m : meshes)
            {
                CVoxelSection voxel(materials);
                voxel.Mesh = m;

                voxel.Serialize(stream);
            }
        }

        for (auto &&p : m_TexturePlanes)
        {
            CTexturePlanes planes;
            planes.Name = p.first;
            planes.Planes = std::get<0>(p.second);
            planes.PlanesInfo = std::get<1>(p.second);
            planes.VoxelSpaceSize = std::get<2>(p.second);

            planes.Serialize(stream);
        }
        
        if(m_SceneTree)
        {
            // Writes the scene tree.
            CSceneTreeSection sceneTree(meshes);
            sceneTree.Tree = m_SceneTree;
            sceneTree.Serialize(stream);
        }

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
                    m_Textures[TextureType::EMISSION] = colors.Colors;
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

                case SectionType::TEXTURE_PLANES:
                {
                    CTexturePlanes planes;
                    planes.Deserialize(m_DataStream);

                    m_TexturePlanes[planes.Name] = std::make_tuple(planes.Planes, planes.PlanesInfo, planes.VoxelSpaceSize);
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