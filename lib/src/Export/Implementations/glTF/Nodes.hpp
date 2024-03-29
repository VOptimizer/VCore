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

#ifndef NODES_HPP
#define NODES_HPP

#include <CJSON/JSON.hpp>
#include <string.h>
#include <VCore/Math/Mat4x4.hpp>
#include <VCore/Math/Vector.hpp>
#include <cstdint>

namespace VCore
{
    namespace GLTF
    {
        enum GLTFTypes
        {
            FLOAT = 5126,
            INT = 5125
        };

        class CAsset
        {
            public:
                void Serialize(CJSON &json) const
                {
                    json.AddPair("version", std::string("2.0"));
                    json.AddPair("generator", std::string("Generated with VCore (https://github.com/VOptimizer/VCore)"));
                }        
        };

        class CScene
        {
            public:
                CScene(std::vector<int> &&_RootNodes) : m_RootNodes(std::move(_RootNodes)) {}

                void Serialize(CJSON &json) const
                {
                    json.AddPair("nodes", m_RootNodes);
                }    
            private:
                std::vector<int> m_RootNodes;
        };

        class CNode
        {
            public:
                CNode(const std::string &_Name) :  m_MeshId(-1), m_Name(_Name) {}
                CNode(const std::string &_Name, int meshId, const Math::Mat4x4 &mat) : m_MeshId(meshId), m_Matrix(mat), m_Name(_Name) {}
                CNode(CNode&& _Other) {*this = std::move(_Other);}

                void Serialize(CJSON &json) const
                {
                    if(!m_Name.empty())
                        json.AddPair("name", m_Name);

                    if(!m_Children.empty())
                        json.AddPair("children", m_Children);

                    if(m_MeshId != -1)
                        json.AddPair("mesh", m_MeshId);
                    std::vector<float> matrix = {
                        m_Matrix.x.x, m_Matrix.y.x, m_Matrix.z.x, m_Matrix.w.x,
                        m_Matrix.x.y, m_Matrix.y.y, m_Matrix.z.y, m_Matrix.w.y,
                        m_Matrix.x.z, m_Matrix.y.z, m_Matrix.z.z, m_Matrix.w.z,
                        m_Matrix.x.w, m_Matrix.y.w, m_Matrix.z.w, m_Matrix.w.w
                    };

                    json.AddPair("matrix", matrix);
                }    

                inline void AddChild(int _NodeId)
                {
                    m_Children.push_back(_NodeId);
                }

                inline CNode &operator=(CNode&& _Other)
                {
                    m_MeshId = _Other.m_MeshId;
                    m_Name = std::move(_Other.m_Name);
                    m_Children = std::move(_Other.m_Children);
                    m_Matrix = _Other.m_Matrix;

                    _Other.m_MeshId = -1;
                    _Other.m_Matrix = Math::Mat4x4();

                    return *this;
                }
            private:
                int m_MeshId;
                Math::Mat4x4 m_Matrix;
                std::string m_Name;
                std::vector<int> m_Children;
        };

        class CBuffer
        {
            public:
                size_t Size;
                std::string Uri;

                void Serialize(CJSON &json) const
                {
                    json.AddPair("byteLength", Size);

                    if(!Uri.empty())
                        json.AddPair("uri", Uri);
                }    
        };

        enum BufferTarget : uint32_t
        {
            NONE = 0,

            ARRAY_BUFFER = 34962,
            ELEMENT_ARRAY_BUFFER = 34963
        };

        class CBufferView
        {
            public:
                CBufferView() : Size(0), Offset(0), Target(BufferTarget::NONE), ByteStride(0) {}

                size_t Size;
                size_t Offset;
                BufferTarget Target;
                size_t ByteStride;

                void Serialize(CJSON &json) const
                {
                    json.AddPair("buffer", 0);
                    json.AddPair("byteLength", Size);
                    json.AddPair("byteOffset", Offset);

                    if(Target != BufferTarget::NONE)
                        json.AddPair("target", (uint32_t)Target);

                    if(ByteStride != 0)
                        json.AddPair("byteStride", ByteStride);
                }    
        };

        class CAccessor
        {
            public:
                CAccessor() : BufferView(0), ComponentType(GLTFTypes::FLOAT), Count(0), Offset(0) {}

                size_t BufferView;
                GLTFTypes ComponentType;
                std::string Type;
                size_t Count;
                size_t Offset;

                inline void SetMax(Math::Vec3f Max)
                {
                    if(Type == "VEC3")
                        m_Max.insert(m_Max.end(), Max.v, Max.v + 3);
                    else if(Type == "VEC2")
                        m_Max.insert(m_Max.end(), Max.v, Max.v + 2);
                    else
                        m_Max.push_back(Max.x);
                }

                inline void SetMin(Math::Vec3f Min)
                {
                    if(Type == "VEC3")
                        m_Min.insert(m_Min.end(), Min.v, Min.v + 3);
                    else if(Type == "VEC2")
                        m_Min.insert(m_Min.end(), Min.v, Min.v + 2);
                    else
                        m_Min.push_back(Min.x);
                }

                void Serialize(CJSON &json) const
                {
                    json.AddPair("bufferView", BufferView);
                    json.AddPair("componentType", (int)ComponentType);

                    if(Offset != 0)
                        json.AddPair("byteOffset", Offset);

                    if(!m_Max.empty())
                        json.AddPair("max", m_Max);

                    if(!m_Min.empty())
                        json.AddPair("min", m_Min);

                    json.AddPair("type", Type);
                    json.AddPair("count", Count);
                }

            private:
                std::vector<float> m_Max, m_Min;
        };

        class CPrimitive
        {
            public:
                CPrimitive() : 
                    NormalAccessor(-1),
                    PositionAccessor(-1),
                    TextCoordAccessor(-1),
                    IndicesAccessor(-1),
                    Material(-1) {}

                int64_t NormalAccessor;
                int64_t PositionAccessor;
                int64_t TextCoordAccessor;
                int64_t IndicesAccessor;
                int64_t Material;

                void Serialize(CJSON &json) const
                {
                    std::map<std::string, int64_t> Attributes;

                    Attributes["NORMAL"] = NormalAccessor;
                    Attributes["POSITION"] = PositionAccessor;
                    Attributes["TEXCOORD_0"] = TextCoordAccessor;

                    json.AddPair("attributes", Attributes);
                    json.AddPair("indices", IndicesAccessor);
                    json.AddPair("material", Material);
                }
        };

        class CMesh
        {
            public:
                std::vector<CPrimitive> Primitives;

                void Serialize(CJSON &json) const
                {
                    json.AddPair("primitives", Primitives);
                }
        };

        class CTexture
        {
            public:
                CTexture() : m_Source(0) {}
                CTexture(int _source) : m_Source(_source) {}

                void Serialize(CJSON &json) const
                {
                    json.AddPair("source", m_Source);
                }

            private:
                int m_Source;
        };

        class CImage
        {
            public:
                std::string Uri;
                int BufferView;

                void Serialize(CJSON &json) const
                {
                    if(!Uri.empty())
                        json.AddPair("uri", Uri);
                    else
                    {
                        json.AddPair("bufferView", BufferView);
                        json.AddPair("mimeType", std::string("image/png"));
                    }
                }
        };

        class CMaterial
        {
            public:
                std::string Name;
                float Roughness;
                float Metallic;
                float Emissive;
                float Transparency;

                void Serialize(CJSON &json) const
                {
                    CJSON PBRMetallicRoughness;

                    std::map<std::string, int> BaseColorTexture = {
                        {"index", 0},
                        {"texCoord", 0}
                    };

                    if(Emissive != 0)
                    {
                        std::map<std::string, int> EmissiveTexture = {
                            {"index", 1},
                            {"texCoord", 0}
                        };

                        json.AddPair("emissiveTexture", EmissiveTexture);
                    }

                    PBRMetallicRoughness.AddPair("baseColorTexture", BaseColorTexture);
                    PBRMetallicRoughness.AddPair("roughnessFactor", Roughness);
                    PBRMetallicRoughness.AddPair("metallicFactor", Metallic);

                    if(Transparency != 0)
                    {
                        json.AddPair("alphaMode", std::string("BLEND"));

                        std::vector<float> baseColorFactor = { 1.f, 1.f, 1.f, 1.f - Transparency };
                        PBRMetallicRoughness.AddPair("baseColorFactor", baseColorFactor);
                    }

                    json.AddPair("name", Name);
                    json.AddJSON("pbrMetallicRoughness", PBRMetallicRoughness.Serialize());
                    json.AddPair("emissiveFactor", std::vector<float>(3, Emissive));
                }
        };
    } // namespace GLTF
    
}

#endif //NODES_HPP