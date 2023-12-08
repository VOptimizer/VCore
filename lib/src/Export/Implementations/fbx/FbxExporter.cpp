#include "FbxExporter.hpp"
#include <time.h>

// Sources:
// This post describes the ascii format of fbx
// https://banexdevblog.wordpress.com/2014/06/23/a-quick-tutorial-about-the-fbx-ascii-format/
// 
// This post contains the basic data structures, which are used in the format
// https://code.blender.org/2013/08/fbx-binary-file-format-specification/
// 
// This wiki seems outdated but describes the neccessary nodes to create a fbx file.
// https://archive.blender.org/wiki/index.php/User:Mont29/Foundation/FBX_File_Structure/#Texture_Data
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
    void CFbxProperty::Serialize(IFileStream *_Stream)
    {
        _Stream->Write(&m_Type, sizeof(char));
        switch (m_Type)
        {
            case 'I': _Stream->Write((char*)&m_Value.iVal, sizeof(int)); break;
            case 'L': _Stream->Write((char*)&m_Value.lVal, sizeof(int64_t)); break;
            case 'D': _Stream->Write((char*)&m_Value.dVal, sizeof(double)); break;

            case 'R':
            case 'S': 
            {
                int size = m_StrValue.size();
                _Stream->Write((char*)&size, sizeof(int));
                _Stream->Write(m_StrValue.data(), m_StrValue.size());
            } break;

            case 'f':
            {
                int size = m_FloatArray.size();
                _Stream->Write((char*)&size, sizeof(int));

                int encoding = 0;
                int compressionLength = size * sizeof(float);
                _Stream->Write((char*)&encoding, sizeof(int));
                _Stream->Write((char*)&compressionLength, sizeof(int));

                _Stream->Write((char*)m_FloatArray.data(), m_FloatArray.size() * sizeof(float));
            } break;

            case 'i':
            {
                int size = m_IntArray.size();
                _Stream->Write((char*)&size, sizeof(int));

                int encoding = 0;
                int compressionLength = size * sizeof(int);
                _Stream->Write((char*)&encoding, sizeof(int));
                _Stream->Write((char*)&compressionLength, sizeof(int));

                _Stream->Write((char*)m_IntArray.data(), m_IntArray.size() * sizeof(int));
            } break;
        }
    }

    void CFbxNode::AddSubNode(CFbxNode &&_Node)
    {
        m_SubNodes.emplace_back(std::move(_Node));
    }

    void CFbxNode::AddSubNode(const std::string &_Name, const std::vector<CFbxProperty> &_Props)
    {
        m_SubNodes.emplace_back(_Name, _Props);
    }

    void CFbxNode::Serialize(IFileStream *_Stream)
    {
        auto startPos = _Stream->Tell();
        uint32_t num = 0;

        // EndOffset
        _Stream->Write((char*)&num, sizeof(num));

        // Num properties
        num = m_Properties.size();
        _Stream->Write((char*)&num, sizeof(num));

        auto propertySizeOffset = _Stream->Tell();
        num = 0;
        // Properties byte size.
        _Stream->Write((char*)&num, sizeof(num));

        uint8_t nameLen = m_Name.size();
        _Stream->Write((char*)&nameLen, sizeof(nameLen));

        if(!m_Name.empty())
            _Stream->Write(m_Name.data(), nameLen);

        if(!m_Properties.empty())
        {
            auto propertiesBegin = _Stream->Tell();
            for (auto &&prop : m_Properties)
                prop.Serialize(_Stream);
            
            auto currentPos = _Stream->Tell();
            _Stream->Seek(propertySizeOffset, SeekOrigin::BEG);

            num = currentPos - propertiesBegin;

            // Patch properties byte size.
            _Stream->Write((char*)&num, sizeof(num));
            _Stream->Seek(currentPos, SeekOrigin::BEG);
        }

        // Write all subnodes
        for (auto &&node : 	m_SubNodes)
            node.Serialize(_Stream);
        
        // Not Zero node.
        if(!m_Name.empty())
        {
            auto currentPos = _Stream->Tell();
            _Stream->Seek(startPos, SeekOrigin::BEG);

            num = startPos + (currentPos - startPos);

            // Patch end offset.
            _Stream->Write((char*)&num, sizeof(num));
            _Stream->Seek(currentPos, SeekOrigin::BEG);
        }
    }

    void CFbxExporter::WriteData(const std::string &_Path, const std::vector<Mesh> &_Meshes)
    {
        auto strm = m_IOHandler->Open(_Path, "wb");
        if(strm)
        {
            // Writes the fbx binary header
            strm->Write(SIGNATURE, sizeof(SIGNATURE) - 1);
            strm->Write(UNKNOWN_HEADER_BYTES, sizeof(UNKNOWN_HEADER_BYTES));
            strm->Write((char*)&FBX_VERSION, sizeof(FBX_VERSION));

            WriteFBXHeader(strm);
            WriteGlobalSettings(strm);

            CFbxNode objects("Objects");
            CFbxNode connections("Connections");
            for (auto &&m : _Meshes)
            {
                AddMesh(objects, m);

                // Creates connections betweend model, geometry, material and the root node 0
                connections.AddSubNode("C", { CFbxProperty("OO"), CFbxProperty(((int64_t)m.get()) + 1), CFbxProperty((int64_t)0) });
                connections.AddSubNode("C", { CFbxProperty("OO"), CFbxProperty((int64_t)m.get()), CFbxProperty(((int64_t)m.get()) + 1) });
            }
            objects.AddSubNode("", {});
            objects.Serialize(strm);

            connections.AddSubNode("", {}); // Zero node
            connections.Serialize(strm);

            WriteFBXFooter(strm);

            m_IOHandler->Close(strm);
        }
    }

    void CFbxExporter::WriteFBXHeader(IFileStream *_Stream)
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

        headerNode.AddSubNode("Creator", { CFbxProperty("Generated with VoxelOptimizer") });
        headerNode.AddSubNode("", {});   // Zero node

        headerNode.Serialize(_Stream);

        CFbxNode fileId("FileId", { CFbxProperty((char*)GENERIC_FILEID, sizeof(GENERIC_FILEID)) });
        CFbxNode creationTime("CreationTime", { CFbxProperty((char*)GENERIC_CTIME, sizeof(GENERIC_CTIME)) });
        CFbxNode creator("Creator", { CFbxProperty("Generated with VoxelOptimizer") });

        fileId.Serialize(_Stream);
        creationTime.Serialize(_Stream);
        creator.Serialize(_Stream);
    }

    void CFbxExporter::WriteGlobalSettings(IFileStream *_Stream)
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
        globalSettings.Serialize(_Stream);
    }

    void CFbxExporter::WriteFBXFooter(IFileStream *_Stream)
    {
        CFbxNode null("");
        null.Serialize(_Stream);

        _Stream->Write((char*)GENERIC_FOOTID, sizeof(GENERIC_FOOTID));

        char czero = 0;

        // Padding for 16 Byte alignment.
        size_t pos = _Stream->Tell();
        size_t pad = 16 - (pos % 16);
        for (size_t i = 0; i < pad; ++i)
            _Stream->Write(&czero, sizeof(czero));

        int zero = 0;
        _Stream->Write((char*)&zero, sizeof(zero));

        _Stream->Write((char*)&FBX_VERSION, sizeof(FBX_VERSION));

        for (size_t i = 0; i < 120; ++i)
            _Stream->Write(&czero, sizeof(czero));

        _Stream->Write((char*)FOOT_MAGIC, sizeof(FOOT_MAGIC));
    }

    void CFbxExporter::AddMesh(CFbxNode &_Objects, Mesh _Mesh)
    {
        // Each mesh consists of a geometry node and a model node.
        // The geometry node contains all informations of a model such as vertices, normals, uvs, material, blend shapes and more.
        // The model contains the transformation data and is linked via the connections with the corresponding geometry node.

        //                                                      | Each object needs a unique id. I'm lazy so I use the "unique id" of the ram. 
        //                                                      v
        CFbxNode geometry("Geometry", { CFbxProperty(((int64_t)_Mesh.get())), CFbxProperty("VoxelModel\x00\x01Geometry", 19, true), CFbxProperty("Mesh") });
        geometry.AddSubNode("Properties70", {});
        geometry.AddSubNode("GeometryVersion", { CFbxProperty((int)0x7C) });

        std::vector<float> vertices;
        std::vector<float> normals;
        std::vector<float> uvs;
        std::vector<int> indices;
        int indexOffset = 0;

        for (auto &&surface : _Mesh->Surfaces)
        {
            for (int i = 0; i < surface.Size(); i++)
            {
                auto vertex = surface[i];
                vertices.push_back(vertex.Pos.x);
                vertices.push_back(vertex.Pos.y);
                vertices.push_back(vertex.Pos.z);

                normals.push_back(vertex.Normal.x);
                normals.push_back(vertex.Normal.y);
                normals.push_back(vertex.Normal.z);

                uvs.push_back(vertex.UV.x);
                uvs.push_back(vertex.UV.y);
            }
            
            int counter = 1;
            for(auto &&i: surface.Indices)
            {
                int idx = indexOffset + i;

                // The last index need to be xored by -1. Since we use triangles instead of quads its every third index.
                if(counter % 3 == 0)
                    idx ^= -1;

                indices.push_back(idx);
                counter++;
            }
            indexOffset += surface.Indices.size();
        }

        // Vertices and it's indices.
        geometry.AddSubNode("Vertices", { CFbxProperty(vertices) });
        geometry.AddSubNode("PolygonVertexIndex", { CFbxProperty(indices) });

        // It's possible to have more than one normal layer, but we only need one.
        CFbxNode normalLayer("LayerElementNormal", { CFbxProperty(0) });
        normalLayer.AddSubNode("Version", { CFbxProperty(101) });
        normalLayer.AddSubNode("Name", { CFbxProperty("") });
        normalLayer.AddSubNode("MappingInformationType", { CFbxProperty("ByVertice") });
        normalLayer.AddSubNode("ReferenceInformationType", { CFbxProperty("Direct") });
        normalLayer.AddSubNode("Normals", { CFbxProperty(normals) });
        normalLayer.AddSubNode("", {});
        geometry.AddSubNode(std::move(normalLayer));

        // Also we can have multiple uv layers.
        CFbxNode uvLayer("LayerElementUV", { CFbxProperty(0) });
        uvLayer.AddSubNode("Version", { CFbxProperty(101) });
        uvLayer.AddSubNode("Name", { CFbxProperty("UVMap") });
        uvLayer.AddSubNode("MappingInformationType", { CFbxProperty("ByVertice") });
        uvLayer.AddSubNode("ReferenceInformationType", { CFbxProperty("Direct") });
        uvLayer.AddSubNode("UV", { CFbxProperty(uvs) });
        uvLayer.AddSubNode("", {});
        geometry.AddSubNode(std::move(uvLayer));

        // Connects different layers and surfaces.
        CFbxNode layer("Layer", { CFbxProperty(0) });
        layer.AddSubNode("Version", { CFbxProperty(100) });

        CFbxNode layerelement("LayerElement");    
        layerelement.AddSubNode("Type", { CFbxProperty("LayerElementNormal") });
        layerelement.AddSubNode("TypedIndex", { CFbxProperty(0) });
        layerelement.AddSubNode("", {});
        layer.AddSubNode(std::move(layerelement));

        layerelement = std::move(CFbxNode("LayerElement"));
        layerelement.AddSubNode("Type", { CFbxProperty("LayerElementUV") });
        layerelement.AddSubNode("TypedIndex", { CFbxProperty(0) });
        layerelement.AddSubNode("", {});
        layer.AddSubNode(std::move(layerelement));

        layer.AddSubNode("", {});

        // Add the layer to the object.
        geometry.AddSubNode(std::move(layer));
        geometry.AddSubNode("", {}); // Zero node.

        _Objects.AddSubNode(std::move(geometry));

        // Creates the model object. The connection is made in ::WriteData.

        //                                                      | Same as above but now I use the next address, which should be fine since it's in the mesh object.
        //                                                      v
        CFbxNode model("Model", { CFbxProperty(((int64_t)_Mesh.get()) + 1), CFbxProperty("VoxelModel\x00\x01Model", 17, true), CFbxProperty("Mesh") });
        model.AddSubNode("Version", { CFbxProperty(232) });
        model.AddSubNode("Properties70", {});
        model.AddSubNode("", {});
        _Objects.AddSubNode(std::move(model));
    }
} // namespace VoxelOptimizer
