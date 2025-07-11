#include "FbxExporter.hpp"
#include "VCore/Misc/unordered_dense.h"
#include <VCore/Export/IExporter.hpp>
#include <VCore/Formats/SceneNode.hpp>
#include <VCore/VConfig.hpp>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <stb_image_write.h>
#include <cstdlib>
#include <VCore/Meshing/MaterialManager.hpp>
#include <string>

// Sources:
// This post describes the ascii format of fbx
// https://banexdevblog.wordpress.com/2014/06/23/a-quick-tutorial-about-the-fbx-ascii-format/
// 
// This post contains the basic data structures, which are used in the format
// https://code.blender.org/2013/08/fbx-binary-file-format-specification/
// 
// This wiki seems outdated but describes the neccessary nodes to create a fbx file.
// https://archive.blender.org/wiki/index.php/User:Mont29/Foundation/FBX_File_Structure/
// 
// Also Blender is good to find issues during the development.
// It's also useful to export models from Blender to fbx and look inside them using a hex editor.

// Tells the parser that this is the binary form of fbx.
static const char SIGNATURE[] = "Kaydara FBX Binary  \0";

// Generic values, which seems to work.
static const char GENERIC_CTIME[] = "1970-01-01 10:00:00:000";
static const unsigned char GENERIC_FILEID[] =
    {0x28, 0xb3, 0x2a, 0xeb, 0xb6, 0x24, 0xcc, 0xc2, 0xbf, 0xc8, 0xb0, 0x2a, 0xa9, 0x2b, 0xfc, 0xf1};
static const unsigned char GENERIC_FOOTID[] =
    {0xfa, 0xbc, 0xab, 0x09, 0xd0, 0xc8, 0xd4, 0x66, 0xb1, 0x76, 0xfb, 0x83, 0x1c, 0xf7, 0x26, 0x7e};
static const unsigned char FOOT_MAGIC[] =
    {0xf8, 0x5a, 0x8c, 0x6a, 0xde, 0xf5, 0xd9, 0x7e, 0xec, 0xe9, 0x0c, 0xe3, 0x75, 0x8f, 0x29, 0x0b};

static const int FBX_VERSION = 7400;
static const char UNKNOWN_HEADER_BYTES[] = {0x1A, 0x00};

const int64_t SECOND = 46186158000;

namespace VCore
{
    void CFbxProperty::Serialize(IFileStream *p_Stream)
    {
        p_Stream->Write(&m_Type, sizeof(char));
        switch (m_Type)
        {
            case 'I': p_Stream->Write((char*)&m_Value.iVal, sizeof(int)); break;
            case 'L': p_Stream->Write((char*)&m_Value.lVal, sizeof(int64_t)); break;
            case 'D': p_Stream->Write((char*)&m_Value.dVal, sizeof(double)); break;

            case 'R':
            case 'S': 
            {
                int size = m_StrValue.size();
                p_Stream->Write((char*)&size, sizeof(int));
                p_Stream->Write(m_StrValue.data(), m_StrValue.size());
            } break;

            case 'f':
            {
                int size = m_FloatArray.size();
                p_Stream->Write((char*)&size, sizeof(int));

                int encoding = 1;
                int compressionLength = 0;

                auto compressedArray = stbi_zlib_compress((unsigned char*)m_FloatArray.data(), m_FloatArray.size() * sizeof(float), &compressionLength, 6);

                p_Stream->Write((char*)&encoding, sizeof(int));
                p_Stream->Write((char*)&compressionLength, sizeof(int));

                p_Stream->Write((char*)compressedArray, compressionLength);
                free(compressedArray);
            } break;

            case 'i':
            {
                int size = m_IntArray.size();
                p_Stream->Write((char*)&size, sizeof(int));

                int encoding = 1;
                int compressionLength = 0;

                auto compressedArray = stbi_zlib_compress((unsigned char*)m_IntArray.data(), m_IntArray.size() * sizeof(int), &compressionLength, 6);

                p_Stream->Write((char*)&encoding, sizeof(int));
                p_Stream->Write((char*)&compressionLength, sizeof(int));

                p_Stream->Write((char*)compressedArray, compressionLength);
                free(compressedArray);
            } break;
        }
    }

    void CFbxNode::AddSubNode(CFbxNode &&p_Node)
    {
        m_SubNodes.emplace_back(std::move(p_Node));
    }

    void CFbxNode::AddSubNode(const std::string &p_Name, const fast_vector<CFbxProperty> &p_Props)
    {
        m_SubNodes.emplace_back(p_Name, p_Props);
    }

    void CFbxNode::Serialize(IFileStream *p_Stream)
    {
        auto startPos = p_Stream->Tell();
        uint32_t num = 0;

        // EndOffset
        p_Stream->Write((char*)&num, sizeof(num));

        // Num properties
        num = m_Properties.size();
        p_Stream->Write((char*)&num, sizeof(num));

        auto propertySizeOffset = p_Stream->Tell();
        num = 0;
        // Properties byte size.
        p_Stream->Write((char*)&num, sizeof(num));

        uint8_t nameLen = m_Name.size();
        p_Stream->Write((char*)&nameLen, sizeof(nameLen));

        if(!m_Name.empty())
            p_Stream->Write(m_Name.data(), nameLen);

        if(!m_Properties.empty())
        {
            auto propertiesBegin = p_Stream->Tell();
            for (auto &&prop : m_Properties)
                prop.Serialize(p_Stream);
            
            auto currentPos = p_Stream->Tell();
            p_Stream->Seek(propertySizeOffset, SeekOrigin::BEG);

            num = currentPos - propertiesBegin;

            // Patch properties byte size.
            p_Stream->Write((char*)&num, sizeof(num));
            p_Stream->Seek(currentPos, SeekOrigin::BEG);
        }

        // Write all subnodes
        for (auto &&node : 	m_SubNodes)
            node.Serialize(p_Stream);
        
        // Not Zero node.
        if(!m_Name.empty())
        {
            auto currentPos = p_Stream->Tell();
            p_Stream->Seek(startPos, SeekOrigin::BEG);

            num = startPos + (currentPos - startPos);

            // Patch end offset.
            p_Stream->Write((char*)&num, sizeof(num));
            p_Stream->Seek(currentPos, SeekOrigin::BEG);
        }
    }

    void CFbxExporter::WriteFBXHeader()
    {
        CFbxNode headerNode("FBXHeaderExtension");
        headerNode.AddSubNode("FBXHeaderVersion", { CFbxProperty(1003) });
        headerNode.AddSubNode("FBXVersion", { CFbxProperty(FBX_VERSION) });
        headerNode.AddSubNode("EncryptionType", { CFbxProperty(0) });

        CFbxNode creationTimestamp("CreationTimeStamp");
        time_t t = time(nullptr);
        tm *tm = localtime(&t);

        creationTimestamp.AddSubNode("Version", { CFbxProperty(1000) });
        creationTimestamp.AddSubNode("Year", { CFbxProperty(tm->tm_year + 1900) });
        creationTimestamp.AddSubNode("Month", { CFbxProperty(tm->tm_mon + 1) });
        creationTimestamp.AddSubNode("Day", { CFbxProperty(tm->tm_mday) });
        creationTimestamp.AddSubNode("Hour", { CFbxProperty(tm->tm_hour) });
        creationTimestamp.AddSubNode("Minute", { CFbxProperty(tm->tm_min) });
        creationTimestamp.AddSubNode("Second", { CFbxProperty(tm->tm_sec) });
        creationTimestamp.AddSubNode("Millisecond", { CFbxProperty(0) });
        creationTimestamp.AddSubNode("", {});   // Zero node

        headerNode.AddSubNode(std::move(creationTimestamp));

        headerNode.AddSubNode("Creator", { CFbxProperty(IExporter::WATERMARK) });
        headerNode.AddSubNode("", {});   // Zero node

        headerNode.Serialize(m_Stream);

        CFbxNode fileId("FileId", { CFbxProperty((char*)GENERIC_FILEID, sizeof(GENERIC_FILEID)) });
        CFbxNode creationTime("CreationTime", { CFbxProperty((char*)GENERIC_CTIME, sizeof(GENERIC_CTIME)) });
        CFbxNode creator("Creator", { CFbxProperty(IExporter::WATERMARK) });

        fileId.Serialize(m_Stream);
        creationTime.Serialize(m_Stream);
        creator.Serialize(m_Stream);
    }

    void CFbxExporter::WriteGlobalSettings()
    {
        CFbxNode globalSettings("GlobalSettings");
        globalSettings.AddSubNode("Version", { CFbxProperty(1000) });

        CFbxNode prop70("Properties70");
        prop70.AddP70("UpAxis", "int", "Integer", "", 1);
        prop70.AddP70("UpAxisSign", "int", "Integer", "", 1);
        prop70.AddP70("FrontAxis", "int", "Integer", "", 2);
        prop70.AddP70("FrontAxisSign", "int", "Integer", "", -1);
        prop70.AddP70("CoordAxis", "int", "Integer", "", 0);
        prop70.AddP70("CoordAxisSign", "int", "Integer", "", -1);
        prop70.AddP70("OriginalUpAxis", "int", "Integer", "", 1);
        prop70.AddP70("OriginalUpAxisSign", "int", "Integer", "", 1);

        prop70.AddP70("OriginalUpAxisSign", "double", "Number", "", 1.0);
        prop70.AddP70("OriginalUnitScaleFactor", "double", "Number", "", 1.0);

        prop70.AddP70("AmbientColor", "ColorRGB", "Color", "", 0.0, 0.0, 0.0);

        prop70.AddP70("DefaultCamera", "KString", "", "", "Producer Perspective");

        prop70.AddP70("TimeMode", "enum", "", "", 11);
        prop70.AddP70("TimeProtocol", "enum", "", "", 2);
        prop70.AddP70("SnapOnFrameMode", "enum", "", "", 0);

        prop70.AddP70("TimeSpanStart", "KTime", "Time", "", (int64_t)0);
        prop70.AddP70("TimeSpanStop", "KTime", "Time", "", SECOND);

        prop70.AddP70("CustomFrameRate", "double", "Number", "", -1.0);

        prop70.AddP70("CurrentTimeMarker", "int", "Integer", "", -1);
        prop70.AddP70("TimeMarker", "Compound", "", "");

        prop70.AddSubNode("", {});
        globalSettings.AddSubNode(std::move(prop70));

        globalSettings.AddSubNode("", {});
        globalSettings.Serialize(m_Stream);
    }

    void CFbxExporter::WriteFBXFooter()
    {
        CFbxNode null("");
        null.Serialize(m_Stream);

        m_Stream->Write((char*)GENERIC_FOOTID, sizeof(GENERIC_FOOTID));

        char czero = 0;

        int zero = 0;
        m_Stream->Write((char*)&zero, sizeof(zero));

        // Padding for 16 Byte alignment.
        auto pos = m_Stream->Tell();
        auto pad = 16 - (pos % 16);
        for (uint64_t i = 0; i < pad; ++i)
            m_Stream->Write(&czero, sizeof(czero));

        m_Stream->Write((char*)&FBX_VERSION, sizeof(FBX_VERSION));

        for (size_t i = 0; i < 120; ++i)
            m_Stream->Write(&czero, sizeof(czero));

        m_Stream->Write((char*)FOOT_MAGIC, sizeof(FOOT_MAGIC));
    }

    void CFbxExporter::AddMaterial(uint8_t p_MaterialHandle)
    {
        auto mat = MaterialManager::GetMaterial(p_MaterialHandle);
        if(!mat)
            mat = MaterialManager::GetMaterial(0);

        CFbxNode material("Material", { CFbxProperty(static_cast<int64_t>(m_BaseIdOffset + p_MaterialHandle)), CFbxProperty("default\x00\x01Material", 17, true), CFbxProperty("") });
        material.AddSubNode("Version", { CFbxProperty(102) });
        material.AddSubNode("ShadingModel", { CFbxProperty("Phong") });
        material.AddSubNode("MultiLayer", { CFbxProperty(0) });

        CFbxNode prop70("Properties70");
        prop70.AddP70("DiffuseColor", "Color", "", "A", 0.8, 0.8, 0.8);
        prop70.AddP70("AmbientColor", "Color", "", "A", 0.8, 0.8, 0.8);
        prop70.AddP70("EmissiveColor", "Color", "", "A", 0.8, 0.8, 0.8);
        prop70.AddP70("SpecularColor", "Color", "", "A", 0.8, 0.8, 0.8);
        // prop70.AddP70("TransparentColor", "Color", "", "A", 1.0, 1.0, 1.0);

        prop70.AddP70("EmissiveFactor", "Number", "", "A", mat->Emission);
        prop70.AddP70("SpecularFactor", "Number", "", "A", mat->Specular);
        prop70.AddP70("TransparencyFactor", "Number", "", "A", 1.0 - mat->Transparency);
        prop70.AddP70("ReflectionFactor", "Number", "", "A", mat->Metallic);
        prop70.AddP70("Shininess", "Number", "", "A", mat->Roughness);

        prop70.AddSubNode("", {});
        material.AddSubNode(std::move(prop70));

        material.AddSubNode("", {});
        m_Objects.AddSubNode(std::move(material));
    }

    void CFbxExporter::CreateNode(const CSceneNodeBase *p_Node)
    {
        auto modelNode = dynamic_cast<const CSceneModelNode*>(p_Node);
        if(modelNode)
        {
            if(modelNode->ModelId >= m_SceneTree->GetModels().size())
                return;

            // Creates a connection between model and geometry
            m_Connections.AddSubNode("C", { CFbxProperty("OO"), CFbxProperty(static_cast<int64_t>(modelNode->ModelId + 1)), CFbxProperty(static_cast<int64_t>(m_ModelCounter)) });

            auto mesh = m_SceneTree->GetModels()[modelNode->ModelId];
            for (auto &&surface : mesh->Surfaces) 
                m_Connections.AddSubNode("C", { CFbxProperty("OO"), CFbxProperty(static_cast<int64_t>(m_BaseIdOffset + surface->MaterialHandle)), CFbxProperty(static_cast<int64_t>(m_ModelCounter)) });
        }

        // Creates the model object.
        std::string className = BuildClassName(!modelNode->Name.empty() ? modelNode->Name : "Unnamed", "Model");

        CFbxNode model("Model", { CFbxProperty(static_cast<int64_t>(m_ModelCounter)), CFbxProperty(className.c_str(), className.size(), true), CFbxProperty((modelNode != nullptr) ? "Mesh" : "Null") });
        model.AddSubNode("Version", { CFbxProperty(232) });
        CFbxNode prop70("Properties70");

        auto rot = modelNode->GetRotation();
        auto scale = modelNode->GetScale();

        prop70.AddP70("Lcl Translation", "Lcl Translation", "", "", modelNode->GetPosition().x, modelNode->GetPosition().y, modelNode->GetPosition().z);
        prop70.AddP70("Lcl Rotation", "Lcl Rotation", "", "", rot.x, rot.y, rot.z);
        prop70.AddP70("Lcl Scaling", "Lcl Scaling", "", "", scale.x, scale.y, scale.z);

        prop70.AddSubNode("", {});
        model.AddSubNode(std::move(prop70));

        model.AddSubNode("", {});
        m_Objects.AddSubNode(std::move(model));

        uint64_t parentId = 0;
        if(!m_NodeStack.empty())
            parentId = m_NodeStack[m_NodeStack.size() - 1];

        // Connect with parent node.
        m_Connections.AddSubNode("C", { CFbxProperty("OO"), CFbxProperty(static_cast<int64_t>(m_ModelCounter)), CFbxProperty(static_cast<int64_t>(parentId)) });

        m_NodeStack.push_back(m_ModelCounter);
        m_ModelCounter++;
    }

    void CFbxExporter::WriteHeaderData(const fast_vector<Mesh> &p_Meshes)
    {
        m_Objects = CFbxNode("Objects");
        m_Connections = CFbxNode("Connections");
        m_MeshCounter = 1;
        m_BaseIdOffset = p_Meshes.size() + 1;
        m_ModelCounter = m_BaseIdOffset + Config::MaxMaterialSlots;

        m_Stream = m_IOHandler->Open(m_Path, "wb");
        
        // Writes the fbx binary header
        m_Stream->Write(SIGNATURE, sizeof(SIGNATURE) - 1);
        m_Stream->Write(UNKNOWN_HEADER_BYTES, sizeof(UNKNOWN_HEADER_BYTES));
        m_Stream->Write((char*)&FBX_VERSION, sizeof(FBX_VERSION));

        WriteFBXHeader();
        WriteGlobalSettings();
    }

    void CFbxExporter::WriteMeshData(const Mesh &p_Mesh)
    {
        // Each mesh consists of a geometry node and a model node.
        // The geometry node contains all informations of a model such as vertices, normals, uvs, material, blend shapes and more.
        // The model contains the transformation data and is linked via the connections with the corresponding geometry node.

        auto className = BuildClassName("Unnamed", "Geometry");

        //                                                                                              | Each object needs a unique id.
        //                                                                                              v
        CFbxNode geometry("Geometry", { CFbxProperty(static_cast<int64_t>(m_MeshCounter++)), CFbxProperty(className.c_str(), className.size(), true), CFbxProperty("Mesh") });
        geometry.AddSubNode("Properties70", {});
        geometry.AddSubNode("GeometryVersion", { CFbxProperty((int)0x7C) });

        fast_vector<float> vertices;
        fast_vector<float> normals;
        fast_vector<float> colors;
        fast_vector<int> indices;
        int indexOffset = 0;

        // Material polygon map.
        fast_vector<int> materials;
        // fast_vector<int> colorIndex;
        // ankerl::unordered_dense::map<uint32_t, uint32_t> colorMap;

        for (auto &&surface : p_Mesh->Surfaces)
        {
            for (uint64_t i = 0; i < surface->GetVertexCount(); i++)
            {
                auto vertex = surface->GetVertex(i);
                vertices.push_back(vertex.Pos.x);
                vertices.push_back(vertex.Pos.y);
                vertices.push_back(vertex.Pos.z);

                normals.push_back(vertex.Normal.x);
                normals.push_back(vertex.Normal.y);
                normals.push_back(vertex.Normal.z);

                // auto it = colorMap.find(vertex.Color);
                // if(it == colorMap.end())
                // {
                //     it = colorMap.insert({vertex.Color, colors.size() / 4}).first;

                CColor c(vertex.Color);
                colors.push_back(c.R / 255.f);
                colors.push_back(c.G / 255.f);
                colors.push_back(c.B / 255.f);
                colors.push_back(1.f);
                // }

                // colorIndex.push_back(it->second);
            }

            auto it = m_AddedMaterials.find(surface->MaterialHandle);
            if(it == m_AddedMaterials.end())
            {
                it = m_AddedMaterials.insert({surface->MaterialHandle, m_AddedMaterials.size()}).first;
                AddMaterial(surface->MaterialHandle);
            }

            int counter = 1;
            for (uint64_t i = 0; i < surface->GetFaceCount() * 3; i++)            
            {
                int idx = indexOffset + surface->GetIndex(i);

                // The last index need to be xored by -1. Since we use triangles instead of quads its every third index.
                if(counter % 3 == 0)
                {
                    // Adds for each polygon the corresponding material
                    materials.push_back(it->second);//m_BaseIdOffset + surface->MaterialHandle);
                    idx ^= -1;
                }

                indices.push_back(idx);
                counter++;
            }
            indexOffset += surface->GetVertexCount();
        }

        // Creates the material layer. Which is just the way to assign different materials to different polygons.
        CFbxNode materialLayer("LayerElementMaterial", { CFbxProperty(0) });
        materialLayer.AddSubNode("Version", { CFbxProperty(101) });
        materialLayer.AddSubNode("Name", { CFbxProperty("material") });
        materialLayer.AddSubNode("MappingInformationType", { CFbxProperty("ByPolygon") });
        materialLayer.AddSubNode("ReferenceInformationType", { CFbxProperty("Direct") });
        materialLayer.AddSubNode("Materials", { CFbxProperty(std::move(materials)) });
        materialLayer.AddSubNode("", {});
        geometry.AddSubNode(std::move(materialLayer));

        // Vertices and it's indices.
        geometry.AddSubNode("Vertices", { CFbxProperty(std::move(vertices)) });
        geometry.AddSubNode("PolygonVertexIndex", { CFbxProperty(std::move(indices)) });

        // It's possible to have more than one normal layer, but we only need one.
        CFbxNode normalLayer("LayerElementNormal", { CFbxProperty(0) });
        normalLayer.AddSubNode("Version", { CFbxProperty(101) });
        normalLayer.AddSubNode("Name", { CFbxProperty("") });
        normalLayer.AddSubNode("MappingInformationType", { CFbxProperty("ByVertice") });
        normalLayer.AddSubNode("ReferenceInformationType", { CFbxProperty("Direct") });
        normalLayer.AddSubNode("Normals", { CFbxProperty(std::move(normals)) });
        normalLayer.AddSubNode("", {});
        geometry.AddSubNode(std::move(normalLayer));

        CFbxNode colorLayer("LayerElementColor", { CFbxProperty(0) });
        colorLayer.AddSubNode("Version", { CFbxProperty(101) });
        colorLayer.AddSubNode("Name", { CFbxProperty("Color") });
        colorLayer.AddSubNode("MappingInformationType", { CFbxProperty("ByVertice") });
        colorLayer.AddSubNode("ReferenceInformationType", { CFbxProperty("Direct") });
        colorLayer.AddSubNode("Colors", { CFbxProperty(std::move(colors)) });
        // colorLayer.AddSubNode("ColorIndex", { CFbxProperty(std::move(colorIndex)) });
        colorLayer.AddSubNode("", {});
        geometry.AddSubNode(std::move(colorLayer));

        // Connects different layers and surfaces.
        CFbxNode layer("Layer", { CFbxProperty(0) });
        layer.AddSubNode("Version", { CFbxProperty(100) });

        CFbxNode layerelement("LayerElement");    
        layerelement.AddSubNode("Type", { CFbxProperty("LayerElementNormal") });
        layerelement.AddSubNode("TypedIndex", { CFbxProperty(0) });
        layerelement.AddSubNode("", {});
        layer.AddSubNode(std::move(layerelement));

        layerelement = CFbxNode("LayerElement");
        layerelement.AddSubNode("Type", { CFbxProperty("LayerElementColor") });
        layerelement.AddSubNode("TypedIndex", { CFbxProperty(0) });
        layerelement.AddSubNode("", {});
        layer.AddSubNode(std::move(layerelement));

        layerelement = CFbxNode("LayerElement");
        layerelement.AddSubNode("Type", { CFbxProperty("LayerElementMaterial") });
        layerelement.AddSubNode("TypedIndex", { CFbxProperty(0) });
        layerelement.AddSubNode("", {});
        layer.AddSubNode(std::move(layerelement));

        layer.AddSubNode("", {});

        // Add the layer to the object.
        geometry.AddSubNode(std::move(layer));
        geometry.AddSubNode("", {}); // Zero node.

        m_Objects.AddSubNode(std::move(geometry));
    }

    void CFbxExporter::WriteFooterData()
    {
        m_AddedMaterials.clear();

        m_Objects.AddSubNode("", {});
        m_Objects.Serialize(m_Stream);

        m_Connections.AddSubNode("", {}); // Zero node
        m_Connections.Serialize(m_Stream);

        m_Objects = CFbxNode();
        m_Connections = CFbxNode();

        WriteFBXFooter();
        m_IOHandler->Close(m_Stream);
    }

    void CFbxExporter::EnterSceneNode(const CSceneNodeBase *p_Node)
    {
        if(p_Node->GetParent())
            CreateNode(p_Node);
    }

    void CFbxExporter::LeaveSceneNode(const CSceneNodeBase *p_Node)
    {
        if(p_Node->GetParent())
            m_NodeStack.pop_back();
    }
}
