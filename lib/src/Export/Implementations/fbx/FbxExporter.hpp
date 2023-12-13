#ifndef FBX_HPP
#define FBX_HPP

#include <iostream>
#include <string>
#include <vector>
#include <VCore/Export/IExporter.hpp>
#include <VCore/Misc/FileStream.hpp>

namespace VCore
{
    class CFbxProperty
    {
        public:
            CFbxProperty(int _Value) : m_Type('I') { m_Value.iVal = _Value; }
            CFbxProperty(int64_t _Value) : m_Type('L') { m_Value.lVal = _Value; }
            CFbxProperty(double _Value) : m_Type('D') { m_Value.dVal = _Value; }
            CFbxProperty(const char *_Value) : m_Type('S'), m_StrValue(_Value) { }
            CFbxProperty(const char *_Value, size_t _Size, bool) : m_Type('S'), m_StrValue(_Value, _Size) { }
            CFbxProperty(const char *_Value, size_t _Size) : m_Type('R'), m_StrValue(_Value, _Size) { }

            CFbxProperty(const std::vector<float> &_Value) : m_Type('f'), m_FloatArray(_Value) { }
            CFbxProperty(const std::vector<int> &_Value) : m_Type('i'), m_IntArray(_Value) { }

            void Serialize(IFileStream *_Stream);

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
            std::vector<float> m_FloatArray;
            std::vector<int> m_IntArray;
    };

    class CFbxNode
    {
        public:
            CFbxNode(const std::string &_Name) : m_Name(_Name) {}
            CFbxNode(const std::string &_Name, const std::vector<CFbxProperty> &_Props) : m_Name(_Name), m_Properties(_Props) {}
            CFbxNode(CFbxNode &&_Other) { *this = std::move(_Other); }

            CFbxNode &operator=(CFbxNode &&_Other)
            {
                m_Name = std::move(_Other.m_Name);
                m_SubNodes = std::move(_Other.m_SubNodes);
                m_Properties = std::move(_Other.m_Properties);

                return *this;
            }

            template<class ...Args>
            void AddP70(Args&& ..._Args)
            {
                CFbxNode p("P");
                p.AddProperties(std::forward<Args>(_Args)...);
                AddSubNode(std::move(p));
            }

            template<class T, class ...Args>
            void AddProperties(T&& _Value, Args&& ..._Args)
            {
                m_Properties.emplace_back(std::forward<T>(_Value));
                AddProperties(std::forward<Args>(_Args)...);
            }
            void AddProperties() {}

            void Serialize(IFileStream *_Stream);

            void AddSubNode(CFbxNode &&_Node);
            void AddSubNode(const std::string &_Name, const std::vector<CFbxProperty> &_Props);

            ~CFbxNode() {}

        private:
            std::string m_Name;
            std::vector<CFbxNode> m_SubNodes;
            std::vector<CFbxProperty> m_Properties;
    };

    class CFbxExporter : public IExporter
    {
        public:
            CFbxExporter()  = default;
            ~CFbxExporter() = default;
        protected:
            void WriteData(const std::string &_Path, const std::vector<Mesh> &_Meshes) override;

        private:
            inline std::string BuildClassName(const std::string &_Name, const std::string &_Class)
            {
                std::string retVal = _Name;
                retVal.insert(retVal.end(), 0);
                retVal.insert(retVal.end(), 1);
                retVal += _Class;
                return retVal;
            }

            void WriteFBXHeader(IFileStream *_Stream);
            void WriteGlobalSettings(IFileStream *_Stream);
            void WriteFBXFooter(IFileStream *_Stream);

            void AddTexture(const std::string &_Path, CFbxNode &_Objects, Texture _Texture, TextureType _Type);
            void AddMesh(CFbxNode &_Objects, CFbxNode &_Connections, int64_t _RootId, Mesh _Mesh);
            void AddMaterial(CFbxNode &_Objects, Material _Material);
            void ConnectTextures(CFbxNode &_Connections, Material _Material, const std::map<TextureType, Texture> &_Textures);
            int64_t CreateNull(CFbxNode &_Objects, const std::string &_Name);
    };
}

#endif