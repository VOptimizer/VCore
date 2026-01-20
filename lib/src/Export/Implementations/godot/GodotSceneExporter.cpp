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

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include "GodotSceneExporter.hpp"
#include <VCore/Formats/SceneNode.hpp>
#include <VCore/Math/Vector.hpp>
#include <VCore/Meshing/Color.hpp>
#include <VCore/VConfig.hpp>
#include "src/FileUtils.hpp"
#include <VCore/Meshing/MaterialManager.hpp>
#include <string>

namespace VCore
{
    const char *GetMaterialClassName(GodotVersion p_GodotVersion)
    {
        if(p_GodotVersion == GodotVersion::GODOT3)
            return "SpatialMaterial";

        return "StandardMaterial3D";
    }

    const char *GetEmissionEnergyName(GodotVersion p_GodotVersion)
    {
        if(p_GodotVersion == GodotVersion::GODOT3)
            return "emission_energy";

        return "emission_energy_multiplier";
    }

    const char *GetNode3DClassName(GodotVersion p_GodotVersion)
    {
        if(p_GodotVersion == GodotVersion::GODOT3)
            return "Spatial";

        return "Node3D";
    }

    std::string GenerateGodot4UID()
    {
        static constexpr char Alphabet[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        auto uid = std::chrono::system_clock::now().time_since_epoch().count();

        std::string result;
        while (uid != 0) 
        {
            result += Alphabet[uid % sizeof(Alphabet)];
            uid /= sizeof(Alphabet);
        }
        return result;
    }

    std::string CGodotSceneExporter::GetNodeName(const CSceneNodeBase *p_Node)
    {
        auto model = dynamic_cast<const CSceneModelNode*>(p_Node);
        if(model)
            return model->Name.empty() ? "MeshInstance3D_" + std::to_string(model->ModelId) : model->Name;

        return GetNode3DClassName(m_GodotVersion);
    }

    std::string CGodotSceneExporter::GetParentName(const CSceneNodeBase *p_Node)
    {
        auto parent = p_Node->GetParent();
        if(!parent)
            return "";

        return std::format("parent=\"{}\"", parent->GetParent() == nullptr ? "." : GetNodeName(parent));
    }

    std::string CGodotSceneExporter::FormatResourceId(uint64_t p_Id)
    {
        if(m_GodotVersion == GodotVersion::GODOT3)
            return std::to_string(p_Id);

        return '"' + std::to_string(p_Id) + '"';
    }

    void CGodotSceneExporter::WriteHeaderData(const fast_vector<Mesh> &p_Meshes)
    {
        m_ModelIndex = Config::MaxMaterialSlots;
        ankerl::unordered_dense::set<uint8_t> materials;
        for (auto &&mesh : p_Meshes) 
        {
            for(auto &&surface: mesh->Surfaces)
                materials.insert(surface->MaterialHandle);
        }

        m_ESCNFile = m_IOHandler->Open(GetFilenameWithoutExt(m_Path) + ".escn", "wb");
        m_ESCNFile->Write(std::format("; {}\n", IExporter::WATERMARK));

        std::string uid;
        if(m_GodotVersion == GodotVersion::GODOT4)
            uid = std::format(" uid=\"uid://{}\"", GenerateGodot4UID());

        // Reserve space for the load_steps, and patch them later on.
        m_ESCNFile->Write(std::format("[gd_scene load_steps={} format={}{}]\n\n", p_Meshes.size() + materials.size() + 1, static_cast<int>(m_GodotVersion), uid));
        for (auto &&mesh : p_Meshes) 
        {
            for(auto &&surface: mesh->Surfaces)
                WriteMaterial(surface->MaterialHandle);
        }
    }

    void CGodotSceneExporter::WriteBytesAsString(const uint8_t* p_Bytes, uint32_t p_Size)
    {
        for (uint32_t i = 0; i < p_Size; i++) 
        {
            m_ESCNFile->Write(std::to_string((int)p_Bytes[i]));
            if(i + 1 < p_Size)
                m_ESCNFile->Write(", ");
        }
    }

    void CGodotSceneExporter::WriteColorAsByteString(const CColor &p_Color)
    {
        float f = p_Color.R / 255.f;
        WriteBytesAsString((uint8_t*)&f, sizeof(float));
        m_ESCNFile->Write(", ");
        f = p_Color.G / 255.f;
        WriteBytesAsString((uint8_t*)&f, sizeof(float));
        m_ESCNFile->Write(", ");
        f = p_Color.B / 255.f;
        WriteBytesAsString((uint8_t*)&f, sizeof(float));
        m_ESCNFile->Write(", ");
        f = p_Color.A / 255.f;
        WriteBytesAsString((uint8_t*)&f, sizeof(float));
    }

    void CGodotSceneExporter::WriteGodot3Surface(const ISurface *p_Surface, uint64_t p_SurfaceIdx)
    {
        const auto totalIndices = p_Surface->GetFaceCount() * 3;
        m_ESCNFile->Write(std::format("surfaces/{} = {{\n", p_SurfaceIdx));

        // Vertex | Normal | Color | Index
        m_ESCNFile->Write(std::format("\"format\": {},\n", 1 | 2 | 8 | 256));

        // Triangles
        m_ESCNFile->Write("\"primitive\": 4,\n");
        m_ESCNFile->Write(std::format("\"material\": SubResource({}),\n", static_cast<int>(p_Surface->MaterialHandle)));
        m_ESCNFile->Write(std::format("\"index_count\": {},\n", totalIndices));
        m_ESCNFile->Write(std::format("\"vertex_count\": {},\n", p_Surface->GetVertexCount()));
        m_ESCNFile->Write("\"blend_shape_data\": [ ],\n");
        m_ESCNFile->Write("\"skeleton_aabb\": [ ],\n");

        auto mat = MaterialManager::GetMaterial(p_Surface->MaterialHandle);
        if(!mat)
            mat = MaterialManager::GetMaterial(0);

        Math::Vec3f minBox(INFINITY, INFINITY, INFINITY), maxBox(0, 0, 0);
        m_ESCNFile->Write("\"array_data\": PoolByteArray(");
        for (uint64_t i = 0; i < p_Surface->GetVertexCount(); i++) 
        {
            auto vertex = p_Surface->GetVertex(i);
            minBox = minBox.min(vertex.Pos);
            maxBox = maxBox.max(vertex.Pos);

            WriteBytesAsString((uint8_t*)&vertex.Pos, sizeof(vertex.Pos));
            m_ESCNFile->Write(", ");
            WriteBytesAsString((uint8_t*)&vertex.Normal, sizeof(vertex.Normal));
            m_ESCNFile->Write(", ");

            auto color = CColor::CreateFromRGBA(vertex.Color);
            color.A = 255 * (1.0f - mat->Transparency);
            WriteColorAsByteString(color);

            if((i + 1) != p_Surface->GetVertexCount())
                m_ESCNFile->Write(", ");
        }
        m_ESCNFile->Write("),\n");

        m_ESCNFile->Write("\"array_index_data\": PoolByteArray(");
        for (uint64_t i = (totalIndices - 1); i >= 0; i--)
        {
            auto idx = p_Surface->GetIndex(i);

            if(totalIndices >= (1 << 16))
                WriteBytesAsString((uint8_t*)&idx, sizeof(idx));
            else
            {
                uint16_t sidx = idx;
                WriteBytesAsString((uint8_t*)&sidx, sizeof(sidx));
            }

            if(i != 0)
                m_ESCNFile->Write(", ");
            else
                break;
        }
        m_ESCNFile->Write("),\n");

        auto size = maxBox - minBox;
        m_ESCNFile->Write(std::format("\"aabb\": AABB({}, {}, {}, {}, {}, {})\n", minBox.x, minBox.y, minBox.z, size.x, size.y, size.z));

        m_ESCNFile->Write("}\n");
    }

//     ERROR: Default index buffer initializer array size (144) does not match format required size (72).
//    at: (drivers/vulkan/rendering_device_vulkan.cpp:4544)

    void CGodotSceneExporter::WriteGodot4Surface(const ISurface *p_Surface)
    {
        constexpr uint64_t ARRAY_COMPRESS_FLAGS_BASE = (12 + 1 + 12);
        constexpr uint64_t ARRAY_FLAG_FORMAT_VERSION_BASE = ARRAY_COMPRESS_FLAGS_BASE + 10;
        constexpr uint64_t ARRAY_FLAG_FORMAT_VERSION_SHIFT = ARRAY_FLAG_FORMAT_VERSION_BASE;
        constexpr uint64_t ARRAY_FLAG_FORMAT_VERSION_MASK = 0xFF;
        constexpr uint64_t ARRAY_FLAG_FORMAT_VERSION_2 = 1ULL << ARRAY_FLAG_FORMAT_VERSION_SHIFT;
		constexpr uint64_t ARRAY_FLAG_FORMAT_CURRENT_VERSION = ARRAY_FLAG_FORMAT_VERSION_2;

        constexpr uint64_t mask = (1ULL << 13) - 1ULL;

        // Vertex | Normal | Tangent | Color | Index
        uint64_t format = 1 | 2 | 4 | 8 | 4096;
        format |= (~mask) & 0;
        format &= ~(ARRAY_FLAG_FORMAT_VERSION_MASK << ARRAY_FLAG_FORMAT_VERSION_SHIFT);
        format |= ARRAY_FLAG_FORMAT_CURRENT_VERSION & (ARRAY_FLAG_FORMAT_VERSION_MASK << ARRAY_FLAG_FORMAT_VERSION_SHIFT);

        const auto totalIndices = p_Surface->GetFaceCount() * 3;
        m_ESCNFile->Write("{\n");
        m_ESCNFile->Write(std::format("\"format\": {},\n", format));

        // Triangles
        m_ESCNFile->Write("\"primitive\": 3,\n");
        m_ESCNFile->Write(std::format("\"index_count\": {},\n", totalIndices));
        m_ESCNFile->Write(std::format("\"vertex_count\": {},\n", p_Surface->GetVertexCount()));
        m_ESCNFile->Write("\"uv_scale\": Vector4(0, 0, 0, 0),\n");
        m_ESCNFile->Write(std::format("\"material\": SubResource(\"{}\"),\n", static_cast<int>(p_Surface->MaterialHandle)));

        Math::Vec3f minBox(INFINITY, INFINITY, INFINITY), maxBox(0, 0, 0);
        m_ESCNFile->Write("\"vertex_data\": PackedByteArray(");

        // 1. Write vertex position
        for (uint64_t i = 0; i < p_Surface->GetVertexCount(); i++) 
        {
            auto vertex = p_Surface->GetVertex(i);
            minBox = minBox.min(vertex.Pos);
            maxBox = maxBox.max(vertex.Pos);

            WriteBytesAsString((uint8_t*)&vertex.Pos, sizeof(vertex.Pos));

            m_ESCNFile->Write(", ");
        }

        // 2. Write normals
        for (uint64_t i = 0; i < p_Surface->GetVertexCount(); i++) 
        {
            auto vertex = p_Surface->GetVertex(i);

            auto o = vertex.Normal.octahedron_encode();
            uint16_t u = (uint16_t)(o.x * 65535.0f);
            uint16_t v = (uint16_t)(o.y * 65535.0f);

            u = std::clamp(u, (uint16_t)0, (uint16_t)0xFFFF);
            v = std::clamp(v, (uint16_t)0, (uint16_t)0xFFFF);

            WriteBytesAsString((uint8_t*)&u, sizeof(u));
            m_ESCNFile->Write(", ");
            WriteBytesAsString((uint8_t*)&v, sizeof(v));
            m_ESCNFile->Write(", ");
        }

        // 3. Write tangents
        for (uint64_t i = 0; i < p_Surface->GetVertexCount(); i++) 
        {
            auto vertex = p_Surface->GetVertex(i);

            auto tan = Math::Vec3f(0, 1, 0).cross(vertex.Normal);
            auto o = tan.octahedron_encode();
            o.y = std::max(o.y, 1.0f / 32767.0f);
            o.y *= 0.5f + 0.5f;

            uint16_t u = (uint16_t)(o.x * 65535.0f);
            uint16_t v = (uint16_t)(o.y * 65535.0f);

            u = std::clamp(u, (uint16_t)0, (uint16_t)0xFFFF);
            v = std::clamp(v, (uint16_t)0, (uint16_t)0xFFFF);
            if(u == 0 && v == 0xFFFF)
                u = 0xFFFF;

            WriteBytesAsString((uint8_t*)&u, sizeof(u));
            m_ESCNFile->Write(", ");
            WriteBytesAsString((uint8_t*)&v, sizeof(v));

            if(i + 1 != p_Surface->GetVertexCount())
                m_ESCNFile->Write(", ");
        }
        m_ESCNFile->Write("),\n");

        auto mat = MaterialManager::GetMaterial(p_Surface->MaterialHandle);
        if(!mat)
            mat = MaterialManager::GetMaterial(0);

        m_ESCNFile->Write("\"attribute_data\": PackedByteArray(");
        for (uint64_t i = 0; i < p_Surface->GetVertexCount(); i++) 
        {
            auto vertex = p_Surface->GetVertex(i);
            auto color = CColor::CreateFromRGBA(vertex.Color);

            color.A = 255 * (1.0f - mat->Transparency);
            WriteBytesAsString((uint8_t*)&color, sizeof(color));

            if(i + 1 != p_Surface->GetVertexCount())
                m_ESCNFile->Write(", ");
        }
        m_ESCNFile->Write("),\n");

        // 4. Write colors
        m_ESCNFile->Write("\"index_data\": PackedByteArray(");
        for (uint64_t i = (totalIndices - 1); i >= 0; i--)
        {
            auto idx = p_Surface->GetIndex(i);
            if(totalIndices >= (1 << 16))
                WriteBytesAsString((uint8_t*)&idx, sizeof(idx));
            else
            {
                uint16_t sidx = idx;
                WriteBytesAsString((uint8_t*)&sidx, sizeof(sidx));
            }

            if(i != 0)
                m_ESCNFile->Write(", ");
            else
                break;
        }
        m_ESCNFile->Write("),\n");

        auto size = maxBox - minBox;
        m_ESCNFile->Write(std::format("\"aabb\": AABB({}, {}, {}, {}, {}, {})\n", minBox.x, minBox.y, minBox.z, size.x, size.y, size.z));

        m_ESCNFile->Write("}");
    }

    void CGodotSceneExporter::EnterSceneNode(const CSceneNodeBase *p_Node)
    {
        std::string nodeType = GetNode3DClassName(m_GodotVersion);
        auto model = dynamic_cast<const CSceneModelNode*>(p_Node);
        if(model)
            nodeType = (m_GodotVersion == GodotVersion::GODOT3) ? "MeshInstance" : "MeshInstance3D";
        
        m_ESCNFile->Write(std::format("[node name=\"{}\" type=\"{}\" {}]\n", GetNodeName(p_Node), nodeType, GetParentName(p_Node)));
        if(model)
            m_ESCNFile->Write(std::format("mesh = SubResource({})\n", FormatResourceId(model->ModelId + Config::MaxMaterialSlots)));

        // Write local transform
        auto matrix = p_Node->GetLocalTransform();
        m_ESCNFile->Write(std::format("transform = Transform({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {})\n",
            matrix.x.x, matrix.y.x, matrix.z.x,
            matrix.x.y, matrix.y.y, matrix.z.y,
            matrix.x.z, matrix.y.z, matrix.z.z,
            matrix.x.w, matrix.y.w, matrix.z.w
        ));
    }

    void CGodotSceneExporter::WriteMeshData(const Mesh &p_Mesh)
    {
        m_ESCNFile->Write(std::format("[sub_resource type=\"ArrayMesh\" id={}]\n", FormatResourceId(m_ModelIndex++)));

        if(m_GodotVersion == GodotVersion::GODOT4)
            m_ESCNFile->Write("_surfaces = [");

        size_t surfaceIdx = 0;
        for(auto &&surface: p_Mesh->Surfaces)
        {
            if(m_GodotVersion == GodotVersion::GODOT3)
                WriteGodot3Surface(surface, surfaceIdx);
            else
            {
                WriteGodot4Surface(surface);
                if((surfaceIdx + 1) != p_Mesh->Surfaces.size())
                    m_ESCNFile->Write(",\n");
                else
                    m_ESCNFile->Write("\n");
            }

            surfaceIdx++;
        }

        if(m_GodotVersion == GodotVersion::GODOT4)
            m_ESCNFile->Write("]\nblend_shape_mode = 0\n");
    }

    void CGodotSceneExporter::WriteMaterial(uint8_t p_MaterialHandle)
    {
        m_ESCNFile->Write(std::format("[sub_resource type=\"{}\" id={}]\n", GetMaterialClassName(m_GodotVersion), FormatResourceId(static_cast<int>(p_MaterialHandle))));

        auto material = MaterialManager::GetMaterial(p_MaterialHandle);
        if(!material)
            material = MaterialManager::GetMaterial(0);

        m_ESCNFile->Write("vertex_color_use_as_albedo = true\n");

        m_ESCNFile->Write(std::format("metallic = {}\n", material->Metallic));
        m_ESCNFile->Write(std::format("metallic_specular = {}\n", material->Specular));
        m_ESCNFile->Write(std::format("roughness = {}\n", material->Roughness));

        if(material->Emission != 0)
        {
            m_ESCNFile->Write("emission_enabled = true\n");
            m_ESCNFile->Write(std::format("{} = {}\n", GetEmissionEnergyName(m_GodotVersion), material->Emission));
            m_ESCNFile->Write("emission = Color(1.0, 1.0, 1.0, 1.0)\n");
        }

        // if(material->IOR != 0)
        // {
        //     m_ESCNFile->Write("refraction_enabled = true\n");
        //     m_ESCNFile->Write(std::format("refraction_scale = {}\n", material->IOR));
        // }

        if(material->Transparency != 0.0)
        {
            if(m_GodotVersion == GodotVersion::GODOT3)
                m_ESCNFile->Write("flags_transparent = true\n");
            else
                m_ESCNFile->Write("transparency = 1\n");
            m_ESCNFile->Write(std::format("albedo_color = Color( 1, 1, 1, {})\n", material->Transparency));
        }
    }

    void CGodotSceneExporter::WriteFooterData()
    {
        m_IOHandler->Close(m_ESCNFile);
        m_ESCNFile = nullptr;
    }
}
