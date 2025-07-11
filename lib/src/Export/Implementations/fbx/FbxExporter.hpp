#ifndef FBX_HPP
#define FBX_HPP

#include "VCore/Misc/fast_vector.hpp"
#include "VCore/Misc/unordered_dense.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <VCore/Export/IExporter.hpp>
#include <VCore/Misc/FileStream.hpp>

namespace VCore
{
    class CFbxProperty
    {
        public:
            CFbxProperty(int p_Value) : m_Type('I') { m_Value.iVal = p_Value; }
            CFbxProperty(int64_t p_Value) : m_Type('L') { m_Value.lVal = p_Value; }
            CFbxProperty(double p_Value) : m_Type('D') { m_Value.dVal = p_Value; }
            CFbxProperty(const char *p_Value) : m_Type('S'), m_StrValue(p_Value) { }
            CFbxProperty(const char *p_Value, size_t p_Size, bool) : m_Type('S'), m_StrValue(p_Value, p_Size) { }
            CFbxProperty(const char *p_Value, size_t p_Size) : m_Type('R'), m_StrValue(p_Value, p_Size) { }

            CFbxProperty(fast_vector<float> &&p_Value) : m_Type('f'), m_FloatArray(std::move(p_Value)) { }
            CFbxProperty(fast_vector<int> &&p_Value) : m_Type('i'), m_IntArray(std::move(p_Value)) { }

            CFbxProperty(const CFbxProperty &p_Other) { *this = p_Other; }
            CFbxProperty(CFbxProperty &&p_Other) { *this = std::move(p_Other); }

            inline CFbxProperty &operator=(const CFbxProperty &p_Other)
            {
                m_Type = p_Other.m_Type;
                m_Value.lVal = p_Other.m_Value.lVal;
                m_StrValue = p_Other.m_StrValue;
                m_FloatArray = p_Other.m_FloatArray;
                m_IntArray = p_Other.m_IntArray;
                return *this;
            }

            inline CFbxProperty &operator=(CFbxProperty &&p_Other)
            {
                m_Type = std::move(p_Other.m_Type);
                m_Value.lVal = std::move(p_Other.m_Value.lVal);
                m_StrValue = std::move(p_Other.m_StrValue);
                m_FloatArray = std::move(p_Other.m_FloatArray);
                m_IntArray = std::move(p_Other.m_IntArray);

                p_Other.m_Type = 0;
                p_Other.m_Value.lVal = 0;

                return *this;
            }

            void Serialize(IFileStream *p_Stream);

        private:
            union Value
            {
                int iVal;
                double dVal;
                intptr_t lVal;
            };

            char m_Type;
            Value m_Value;  

            std::string m_StrValue;
            fast_vector<float> m_FloatArray;
            fast_vector<int> m_IntArray;
    };

    class CFbxNode
    {
        public:
            CFbxNode() {}
            CFbxNode(const std::string &p_Name) : m_Name(p_Name) {}
            CFbxNode(const std::string &p_Name, const fast_vector<CFbxProperty> &p_Props) : m_Name(p_Name), m_Properties(p_Props) {}
            CFbxNode(CFbxNode &&p_Other) { *this = std::move(p_Other); }

            CFbxNode &operator=(CFbxNode &&p_Other)
            {
                m_Name = std::move(p_Other.m_Name);
                m_SubNodes = std::move(p_Other.m_SubNodes);
                m_Properties = std::move(p_Other.m_Properties);

                return *this;
            }

            template<class ...Args>
            void AddP70(Args&& ...p_Args)
            {
                CFbxNode p("P");
                p.AddProperties(std::forward<Args>(p_Args)...);
                AddSubNode(std::move(p));
            }

            template<class T, class ...Args>
            void AddProperties(T&& p_Value, Args&& ...p_Args)
            {
                m_Properties.emplace_back(std::forward<T>(p_Value));
                AddProperties(std::forward<Args>(p_Args)...);
            }
            void AddProperties() {}

            void Serialize(IFileStream *p_Stream);

            void AddSubNode(CFbxNode &&p_Node);
            void AddSubNode(const std::string &p_Name, const fast_vector<CFbxProperty> &p_Props);

            ~CFbxNode() {}

        private:
            std::string m_Name;
            fast_vector<CFbxNode> m_SubNodes;
            fast_vector<CFbxProperty> m_Properties;
    };

    class CFbxExporter : public IExporter
    {
        public:
            CFbxExporter()  = default;
            ~CFbxExporter() = default;
        protected:
            void WriteHeaderData(const fast_vector<Mesh> &) override;
            bool SupportsSceneTree() override { return true; }
            void WriteMeshData(const Mesh &p_Mesh) override;
            void WriteFooterData() override;

            void EnterSceneNode(const CSceneNodeBase *p_Node) override;
            void LeaveSceneNode(const CSceneNodeBase *) override;

        private:
            IFileStream *m_Stream{};
            CFbxNode m_Objects, m_Connections;
            uint64_t m_MeshCounter{};
            uint64_t m_BaseIdOffset{};
            uint64_t m_ModelCounter{};
            ankerl::unordered_dense::map<uint8_t, uint32_t> m_AddedMaterials;
            fast_vector<uint64_t> m_NodeStack;

            inline std::string BuildClassName(const std::string &p_Name, const std::string &p_Class)
            {
                std::string retVal = p_Name;
                retVal.insert(retVal.end(), 0);
                retVal.insert(retVal.end(), 1);
                retVal += p_Class;
                return retVal;
            }

            void WriteFBXHeader();
            void WriteGlobalSettings();
            void WriteFBXFooter();

            void AddMaterial(uint8_t p_MaterialHandl);
            void CreateNode(const CSceneNodeBase *p_Node);
    };
}

#endif