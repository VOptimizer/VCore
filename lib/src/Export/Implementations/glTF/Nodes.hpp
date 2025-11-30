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

#include "VCore/Export/IExporter.hpp"
#include <CJSON/JSON.hpp>
#include <cstring>
#include <VCore/Math/Mat4x4.hpp>
#include <VCore/Math/Vector.hpp>
#include <cstdint>
#include <VCore/Misc/fast_vector.hpp>
#include <VCore/Misc/unordered_dense.h>

#include <VCore/Meshing/Material.hpp>
#include <VCore/Formats/SceneNode.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <string_view>

namespace VCore::GLTF
{
    enum class GLTFTypes
    {
        UNSIGNED_BYTE = 5121,
        INT = 5125,
        FLOAT = 5126,
    };

    class CAsset
    {
        public:
            void Serialize(CJSON &p_Json) const
            {
                p_Json.AddPair("version", std::string("2.0"));
                p_Json.AddPair("generator", std::string(IExporter::WATERMARK));
            }        
    };

    class CScene
    {
        public:
            explicit CScene(const fast_vector<uint64_t> &p_RootNodes) : m_RootNodes(p_RootNodes) {}

            void Serialize(CJSON &p_Json) const
            {
                p_Json.AddPair("nodes", m_RootNodes);
            }    
        private:
            const fast_vector<uint64_t> &m_RootNodes;
    };

    class CNode
    {
        public:
            explicit CNode(const std::string &p_Name) : m_MeshId(-1), m_Name(p_Name) {}
            CNode(
                const std::string &p_Name, 
                uint64_t p_meshId, 
                const Math::Vec3f &p_Position, 
                const Math::Vec3f &p_Rotation, 
                const Math::Vec3f &p_Scale) : 
                m_MeshId(p_meshId), m_Name(p_Name), m_Position(p_Position), m_Rotation(p_Rotation), m_Scale(p_Scale) {}
            CNode(CNode&& p_Other) noexcept : m_MeshId(-1) {*this = std::move(p_Other);}
            CNode(const CNode &) = delete;

            void Serialize(CJSON &p_json) const
            {
                if(!m_Name.empty())
                    p_json.AddPair("name", m_Name);

                if(!m_Children.empty())
                    p_json.AddPair("children", m_Children);

                if(m_MeshId != static_cast<uint64_t>(-1))
                    p_json.AddPair("mesh", m_MeshId);

                p_json.AddPair("rotation", EulerToQuaternion(m_Rotation));
                p_json.AddPair("translation", fast_vector<float>({m_Position.x, m_Position.y, m_Position.z}));
                p_json.AddPair("scale", fast_vector<float>({m_Scale.x, m_Scale.y, m_Scale.z}));
            }    

            inline void AddChild(uint64_t p_NodeId)
            {
                m_Children.push_back(p_NodeId);
            }

            CNode &operator=(const CNode &) = delete;
            inline CNode &operator=(CNode&& p_Other) noexcept
            {
                m_MeshId = p_Other.m_MeshId;
                m_Name = std::move(p_Other.m_Name);
                m_Children = std::move(p_Other.m_Children);
                m_Position = p_Other.m_Position;
                m_Rotation = p_Other.m_Rotation;
                m_Scale = p_Other.m_Scale;

                p_Other.m_MeshId = -1;
                m_Position = Math::Vec3f();
                m_Rotation = Math::Vec3f();
                m_Scale = Math::Vec3f();

                return *this;
            }

            ~CNode() = default;
        private:
            static fast_vector<float> EulerToQuaternion(const Math::Vec3f &p_Euler)
            {
                if(p_Euler == Math::Vec3f::ZERO)
                    return { 0, 0, 0, 1 };
        
                auto halfAngle = p_Euler * 0.5f;
                Math::Vec3f c(cos(halfAngle.x), cos(halfAngle.y), cos(halfAngle.z));
                Math::Vec3f s(sin(halfAngle.x), sin(halfAngle.y), sin(halfAngle.z));
        
                return { 
                    s.x * c.y * c.z - c.x * s.y * s.z,
                    c.x * s.y * c.z + s.x * c.y * s.z,
                    c.x * c.y * s.z - s.x * s.y * c.z,
                    c.x * c.y * c.z + s.x * s.y * s.z
                };
            }

            uint64_t m_MeshId;
            std::string m_Name;
            fast_vector<uint64_t> m_Children;
            Math::Vec3f m_Position;
            Math::Vec3f m_Rotation;
            Math::Vec3f m_Scale;
    };

    class CBuffer
    {
        public:
            CBuffer(const uint64_t p_Size, const std::string &p_Uri) : Size(p_Size), Uri(p_Uri) {}
            CBuffer(CBuffer &&p_Other) { *this = std::move(p_Other); }
            CBuffer(const CBuffer &) = delete;

            uint64_t Size;
            std::string Uri;

            CBuffer &operator=(const CBuffer &) = delete;
            CBuffer &operator=(CBuffer &&p_Other)
            {
                Size = p_Other.Size;
                Uri = std::move(p_Other.Uri);
                p_Other.Size = 0;

                return *this;
            }

            void Serialize(CJSON &p_Json) const
            {
                p_Json.AddPair("byteLength", Size);

                if(!Uri.empty())
                p_Json.AddPair("uri", Uri);
            }    
    };

    enum class BufferTarget : uint16_t
    {
        NONE = 0,

        ARRAY_BUFFER = 34962,
        ELEMENT_ARRAY_BUFFER = 34963
    };

    class CBufferView
    {
        public:
            explicit CBufferView(
                const uint64_t p_Size, 
                const uint64_t p_Offset, 
                const BufferTarget p_Target, 
                const uint64_t p_Stride) : Size(p_Size), Offset(p_Offset), Target(p_Target), ByteStride(p_Stride) {}

            CBufferView(const CBufferView &) = delete;
            CBufferView(CBufferView &&p_Other) { *this = std::move(p_Other); }

            CBufferView &operator=(const CBufferView &) = delete;
            CBufferView &operator=(CBufferView &&p_Other)
            {
                Size = p_Other.Size;
                Offset = p_Other.Offset;
                Target = p_Other.Target;
                ByteStride = p_Other.ByteStride;

                p_Other.Size = 0;
                p_Other.Offset = 0;
                p_Other.Target = BufferTarget::NONE;
                p_Other.ByteStride = 0;

                return *this;
            }

            uint64_t Size;
            uint64_t Offset;
            BufferTarget Target;
            uint64_t ByteStride;

            void Serialize(CJSON &p_json) const
            {
                p_json.AddPair("buffer", 0);
                p_json.AddPair("byteLength", Size);
                p_json.AddPair("byteOffset", Offset);

                if(Target != BufferTarget::NONE)
                    p_json.AddPair("target", static_cast<uint32_t>(Target));

                if(ByteStride != 0)
                    p_json.AddPair("byteStride", ByteStride);
            }    
    };

    class CAccessor
    {
        public:
            CAccessor(
                const uint64_t p_BufferView,
                const GLTFTypes p_ComponentType,
                const std::string_view p_Type,
                const uint64_t p_Count
            ) : BufferView(p_BufferView), ComponentType(p_ComponentType), Type(p_Type), Count(p_Count) {}

            CAccessor(const CAccessor &p_Other) { *this = p_Other; }
            CAccessor(CAccessor &&p_Other) { *this = std::move(p_Other); }

            CAccessor &operator=(const CAccessor &p_Other)
            {
                BufferView = p_Other.BufferView;
                ComponentType = p_Other.ComponentType;
                Type = p_Other.Type;
                Count = p_Other.Count;
                Offset = p_Other.Offset;
                m_Min = p_Other.m_Min;
                m_Max = p_Other.m_Max;

                return *this;
            }

            CAccessor &operator=(CAccessor &&p_Other)
            {
                BufferView = p_Other.BufferView;
                ComponentType = p_Other.ComponentType;
                Type = std::move(p_Other.Type);
                Count = p_Other.Count;
                Offset = p_Other.Offset;
                m_Min = std::move(p_Other.m_Min);
                m_Max = std::move(p_Other.m_Max);

                p_Other.BufferView = 0;
                p_Other.ComponentType = GLTFTypes::UNSIGNED_BYTE;
                p_Other.Count = 0;
                p_Other.Offset = 0;

                return *this;
            }

            uint64_t BufferView{};
            GLTFTypes ComponentType{};
            std::string Type;
            uint64_t Count{};
            uint64_t Offset{};

            void SetMax(Math::Vec3f p_Max)
            {
                if(Type == "VEC3")
                    m_Max.insert(m_Max.end(), p_Max.v, p_Max.v + 3);
                else if(Type == "VEC2")
                    m_Max.insert(m_Max.end(), p_Max.v, p_Max.v + 2);
                else
                    m_Max.push_back(p_Max.x);
            }

            void SetMin(Math::Vec3f p_Min)
            {
                if(Type == "VEC3")
                    m_Min.insert(m_Min.end(), p_Min.v, p_Min.v + 3);
                else if(Type == "VEC2")
                    m_Min.insert(m_Min.end(), p_Min.v, p_Min.v + 2);
                else
                    m_Min.push_back(p_Min.x);
            }

            void Serialize(CJSON &p_json) const
            {
                p_json.AddPair("bufferView", BufferView);
                p_json.AddPair("componentType", (int)ComponentType);

                if(Offset != 0)
                    p_json.AddPair("byteOffset", Offset);

                if(!m_Max.empty())
                    p_json.AddPair("max", m_Max);

                if(!m_Min.empty())
                    p_json.AddPair("min", m_Min);

                if(Type == "VEC4" && ComponentType == GLTFTypes::UNSIGNED_BYTE)
                    p_json.AddPair("normalized", true);

                p_json.AddPair("type", Type);
                p_json.AddPair("count", Count);
            }

            ~CAccessor()
            {
                Type.clear();
            }
        private:
            fast_vector<float> m_Max, m_Min;
    };

    class CPrimitive
    {
        public:
            explicit CPrimitive(uint64_t p_MaterialHandle) : m_MaterialHandle(p_MaterialHandle) {}
            CPrimitive(const CPrimitive&) = delete;
            CPrimitive(CPrimitive &&p_Other) { *this = std::move(p_Other); }

            CPrimitive &operator=(const CPrimitive&) = delete;
            CPrimitive &operator=(CPrimitive &&p_Other)
            {
                IndicesAccessor = p_Other.IndicesAccessor;
                m_MaterialHandle = p_Other.m_MaterialHandle;
                m_Attributes = std::move(p_Other.m_Attributes);

                p_Other.IndicesAccessor = 0;
                p_Other.m_MaterialHandle = 0;

                return *this;
            }

            uint64_t IndicesAccessor{};

            /** @brief Adds an attribute accessor to this primitive. */
            void AddAttribute(const char *p_Name, uint64_t p_AttributeHandle) { m_Attributes.AddPair(p_Name, p_AttributeHandle); }
            // m_Attributes[p_Name] = p_AttributeHandle;

            void Serialize(CJSON &p_Json) const
            {
                p_Json.AddJSON("attributes", m_Attributes.Serialize());
                p_Json.AddPair("indices", IndicesAccessor);
                p_Json.AddPair("material", m_MaterialHandle);
            }

        private:
            uint64_t m_MaterialHandle;
            mutable CJSON m_Attributes;
    };

    class GLTFDocument;

    class CMesh
    {
        public:
            using AccessorValue = std::pair<const char*, CAccessor>;

            explicit CMesh(GLTFDocument *p_DocumentRef, const uint64_t p_MeshHandle) : m_DocumentRef(p_DocumentRef), m_MeshHandle(p_MeshHandle) {}
            CMesh(CMesh &&p_Other) { *this = std::move(p_Other); }
            CMesh(const CMesh &) = default;

            CMesh &operator=(const CMesh &) = default;
            CMesh &operator=(CMesh &&p_Other)
            {
                m_Primitives = std::move(p_Other.m_Primitives);
                m_DocumentRef = p_Other.m_DocumentRef;
                m_MeshHandle = p_Other.m_MeshHandle;

                p_Other.m_DocumentRef = nullptr;
                p_Other.m_MeshHandle = 0;

                return *this;
            }

            [[nodiscard]] uint64_t GetMeshHandle() const { return m_MeshHandle; }

            /**
             * @brief Adds a primitive, similar to VCores surface, to the mesh
             * @param p_MaterialHandle Handle of the material, which is used to render this surface
             * @param p_Accessors Array of accessors, for the mesh data
             */
            void AddPrimitive(uint64_t p_MaterialHandle, const fast_vector<AccessorValue> &p_Accessors);

            void Serialize(CJSON &p_Json) const
            {
                p_Json.AddPair("primitives", m_Primitives);
            }

        private:
            static uint32_t GetTypeSize(const CAccessor &p_Accessor) 
            {
                if(p_Accessor.Type == "VEC2")
                    return GetPrimitiveTypeSize(p_Accessor.ComponentType) * 2;
                
                if(p_Accessor.Type == "VEC3")
                    return GetPrimitiveTypeSize(p_Accessor.ComponentType) * 3;

                if(p_Accessor.Type == "VEC4")
                    return GetPrimitiveTypeSize(p_Accessor.ComponentType) * 4;

                return GetPrimitiveTypeSize(p_Accessor.ComponentType);
            }

            static uint8_t GetPrimitiveTypeSize(const GLTFTypes p_Type) 
            {
                switch (p_Type) 
                {
                    case GLTFTypes::UNSIGNED_BYTE: return 1;
                    default: return 4;
                }
            }

            fast_vector<CPrimitive> m_Primitives;
            GLTFDocument *m_DocumentRef;
            uint64_t m_MeshHandle;
    };

    // class CTexture
    // {
    //     public:
    //         CTexture() : m_Source(0) {}
    //         CTexture(int p_Source) : m_Source(p_Source) {}

    //         void Serialize(CJSON &p_Json) const
    //         {
    //             p_Json.AddPair("source", m_Source);
    //         }

    //     private:
    //         int m_Source;
    // };

    // class CImage
    // {
    //     public:
    //         std::string Uri;
    //         int BufferView;

    //         void Serialize(CJSON &p_Json) const
    //         {
    //             if(!Uri.empty())
    //                 p_Json.AddPair("uri", Uri);
    //             else
    //             {
    //                 p_Json.AddPair("bufferView", BufferView);
    //                 p_Json.AddPair("mimeType", std::string("image/png"));
    //             }
    //         }
    // };

    class CMaterial
    {
        public:
            explicit CMaterial(const VCore::CMaterial *p_MaterialRef) : MaterialRef(p_MaterialRef) {}
            CMaterial(const CMaterial&) = delete;
            CMaterial(CMaterial &&p_Other) { *this = std::move(p_Other); }

            CMaterial &operator=(const CMaterial&) = delete;
            CMaterial &operator=(CMaterial &&p_Other)
            {
                MaterialRef = p_Other.MaterialRef;
                p_Other.MaterialRef = nullptr;

                return *this;
            }

            const VCore::CMaterial *MaterialRef;

            void Serialize(CJSON &p_Json) const
            {
                CJSON pbrMetallicRoughness;
                pbrMetallicRoughness.AddPair("roughnessFactor", MaterialRef->Roughness);
                pbrMetallicRoughness.AddPair("metallicFactor", MaterialRef->Metallic);

                if(MaterialRef->Transparency != 0.0)
                {
                    p_Json.AddPair("alphaMode", std::string("BLEND"));

                    fast_vector<float> baseColorFactor = { 1.f, 1.f, 1.f, 1.f - MaterialRef->Transparency };
                    pbrMetallicRoughness.AddPair("baseColorFactor", baseColorFactor);
                }

                if(!MaterialRef->Name.empty())
                    p_Json.AddPair("name", MaterialRef->Name);
                p_Json.AddJSON("pbrMetallicRoughness", pbrMetallicRoughness.Serialize());
                p_Json.AddPair("emissiveFactor", fast_vector<float>(3, MaterialRef->Emission));
            }
    };

    class GLTFDocument
    {
        friend CMesh;

        public:
            GLTFDocument() = default;

            /** 
             * @brief Adds a material to this gltf document.
             * @return Returns a handle to the material.
             */
            uint64_t AddMaterial(const VCore::CMaterial *p_Material) 
            {
                m_Materials.push_back(GLTF::CMaterial(p_Material));
                return m_Materials.size() - 1;
            }

            /**
             * @brief Creates a new mesh
             * @return Returns a reference to the newly created mesh. Note: The reference can change if the array resizes, please don't store this reference
             */
            CMesh &CreateMesh()
            {
                return m_Meshes.emplace_back(this, m_Meshes.size());
            }

            /** 
              * @brief Creates a new node 
              * @return Returns a handle to the node.
              */
            uint64_t CreateNode(const CSceneNodeBase *p_Node, uint32_t p_ModelIdDec, const uint64_t p_ParentNode)//const std::string &p_Name, const uint64_t p_MeshHandle, const Math::Mat4x4 &p_Matrix, const uint64_t p_ParentNode)
            {
                uint64_t meshHandle = static_cast<uint64_t>(-1);
                const CSceneModelNode *modelNode = dynamic_cast<const CSceneModelNode*>(p_Node);
                if(modelNode)
                    meshHandle = modelNode->ModelId - p_ModelIdDec;

                m_Nodes.emplace_back(p_Node->Name, meshHandle, p_Node->GetPosition(), p_Node->GetRotation(), p_Node->GetScale());

                auto handle = m_Nodes.size() - 1;
                if(p_ParentNode != static_cast<uint64_t>(-1))
                    m_Nodes[p_ParentNode].AddChild(handle);
                else
                    m_RootNodes.push_back(handle);

                if(!modelNode)
                {
                    const CSceneAnimationNode *animationNode = dynamic_cast<const CSceneAnimationNode*>(p_Node);
                    if(animationNode)
                    {
                        auto &parentNode = m_Nodes[handle];
                        for (auto &&model : animationNode->Frames) 
                        {
                            auto nodeHandle = m_Nodes.size();
                            m_Nodes.emplace_back(p_Node->Name + "_" + std::to_string(model.FrameIdx), model.ModelId, Math::Vec3f(), Math::Vec3f(), Math::Vec3f(1, 1, 1));
                            parentNode.AddChild(nodeHandle);
                        }
                    }
                }

                return handle;
            }

            /**
             * @brief Creates a new buffer view.
             * @return Returns the handle to the buffer view.
             */
            uint64_t CreateBufferView(const uint64_t p_Size, const uint64_t p_Offset,  const BufferTarget p_Target, const uint64_t p_Stride)
            {
                m_BufferViews.emplace_back(p_Size, p_Offset, p_Target, p_Stride);
                return m_BufferViews.size() - 1;
            }

            /** @brief Adds a buffer to the document. */
            void AddBuffer(const uint64_t p_Size, const std::string &p_Uri = "")
            {
                m_Buffers.emplace_back(p_Size, p_Uri);
            }

            void Serialize(CJSON &p_Json) const
            {
                p_Json.AddPair("asset", GLTF::CAsset()); // Metadata + Shameless advertisment
                p_Json.AddPair("scene", 0); // Default Scene

                CJSON scene;
                p_Json.AddJSON("scenes", "[" + scene.Serialize(CScene(m_RootNodes)) + "]");
                p_Json.AddPair("materials", m_Materials);
                p_Json.AddPair("meshes", m_Meshes);
                p_Json.AddPair("accessors", m_Accessors);
                p_Json.AddPair("bufferViews", m_BufferViews);
                p_Json.AddPair("nodes", m_Nodes);
                p_Json.AddPair("buffers", m_Buffers);
            }

            void Clear()
            {
                m_RootNodes.clear();
                m_Accessors.clear();
                m_Meshes.clear();
                m_Materials.clear();
                m_BufferViews.clear();
                m_Nodes.clear();
                m_Buffers.clear();
            }
        private:
            /** @return Returns the handle to the accessor */
            uint64_t AddAccessor(const CAccessor &p_Accessor)
            {
                m_Accessors.push_back(p_Accessor);
                return m_Accessors.size() - 1;
            }

            fast_vector<uint64_t> m_RootNodes;
            fast_vector<CMesh> m_Meshes;
            fast_vector<CAccessor> m_Accessors;
            fast_vector<CMaterial> m_Materials;
            fast_vector<CBufferView> m_BufferViews;
            fast_vector<CNode> m_Nodes;
            fast_vector<CBuffer> m_Buffers;
    };
} // namespace VCore::GLTF

#endif //NODES_HPP