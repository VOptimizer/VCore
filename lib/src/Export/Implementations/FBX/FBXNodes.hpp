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

#ifndef FBXNODES_HPP
#define FBXNODES_HPP

#include <chrono>
#include "../../Streams/BinaryStream.hpp"
#include <VoxelOptimizer/Color.hpp>
#include <string>
#include <string.h>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#if _WIN64 || __x86_64__ || __ppc64__
using double64 = double;
#else
using double64 = long double;
#endif

namespace VoxelOptimizer
{
    const int64_t UUID = 0xdeadbeef;

    class IFBXProperty 
    {
        public:
            virtual void Serialize(CBinaryStream &strm) = 0;
            virtual int Size() = 0;
    };
    using FBXProperty = std::shared_ptr<IFBXProperty>;

    class CFBXNode;
    using FBXNode = std::shared_ptr<CFBXNode>;

    template<class T>
    class CFBXBaseProperty : public IFBXProperty
    {
        public:
            CFBXBaseProperty(const T &val) : Value(val) {}
            T Value;
    };

    template<class T>
    class CFBXProperty : public CFBXBaseProperty<std::string>
    {
        public:
            CFBXProperty(const std::string &val) : CFBXBaseProperty(val) {}

            void Serialize(CBinaryStream &strm) override
            {
                // Value
                strm << 'S';
                strm << Value;
            }

            int Size() override
            {
                return 1 + sizeof(int) + Value.size();
            }
    };

    template<>
    class CFBXProperty<float> : public CFBXBaseProperty<float>
    {
        public:
            CFBXProperty(float val) : CFBXBaseProperty(val) {}

            void Serialize(CBinaryStream &strm) override
            {
                // Value
                strm << 'F';
                strm << Value;
            }

            int Size() override
            {
                return 1 + sizeof(float);
            }
    };

    template<>
    class CFBXProperty<double64> : public CFBXBaseProperty<double64>
    {
        public:
            CFBXProperty(double64 val) : CFBXBaseProperty(val) {}

            void Serialize(CBinaryStream &strm) override
            {
                // Value
                strm << 'D';
                strm << Value;
            }

            int Size() override
            {
                return 1 + sizeof(double64);
            }
    };

    template<>
    class CFBXProperty<CColor> : public CFBXBaseProperty<CColor>
    {
        public:
            CFBXProperty(CColor val) : CFBXBaseProperty(val) {}

            void Serialize(CBinaryStream &strm) override
            {
                // Value
                strm << 'D';
                strm << Value.R / 255.f;

                strm << 'D';
                strm << Value.G / 255.f;

                strm << 'D';
                strm << Value.B / 255.f;
            }

            int Size() override
            {
                return 3 + 3 * sizeof(double64);
            }
    };

    template<>
    class CFBXProperty<int> : public CFBXBaseProperty<int>
    {
        public:
            CFBXProperty(int val) : CFBXBaseProperty(val) {}

            void Serialize(CBinaryStream &strm) override
            {
                // Value
                strm << 'I';
                strm << Value;
            }

            int Size() override
            {
                return 1 + sizeof(int);
            }
    };

    template<>
    class CFBXProperty<int64_t> : public CFBXBaseProperty<int64_t>
    {
        public:
            CFBXProperty(int64_t val) : CFBXBaseProperty(val) {}

            void Serialize(CBinaryStream &strm) override
            {
                // Value
                strm << 'L';
                strm << Value;
            }

            int Size() override
            {
                return 1 + sizeof(int64_t);
            }
    };

    template<>
    class CFBXProperty<const char*> : public CFBXBaseProperty<const char*>
    {
        public:
            CFBXProperty(const char *val, uint32_t size) : CFBXBaseProperty(val), m_Size(size) {}

            void Serialize(CBinaryStream &strm) override
            {
                // Value
                strm << 'R';
                strm.write((char*)&m_Size, sizeof(m_Size));
                strm.write(Value, m_Size);
            }

            int Size() override
            {
                return 1 + sizeof(uint32_t) + m_Size;
            }
        private:
            uint32_t m_Size;
    };

    class CFBXNode
    {
        public:
            CFBXNode() : m_Size(sizeof(uint32_t) * 3 + sizeof(uint8_t) + 13), m_PropertiesSize(0) {}

            void AddProperty(FBXProperty prop)
            {
                m_Properties.push_back(prop);
                m_PropertiesSize += prop->Size();
                m_Size += prop->Size();
            }

            void AddNestedNode(FBXNode node)
            {
                m_NestedNodes.push_back(node);
                m_Size += node->m_Size;
            }

            void SetName(const std::string &Name)
            {
                m_Name = Name;
                m_Size += Name.size();
            }

            virtual void Serialize(CBinaryStream &strm)
            {
                // Endoffset
                strm << (uint32_t)(((uint32_t)strm.offset()) + m_Size);
                strm << (uint32_t)m_Properties.size();
                strm << m_PropertiesSize;

                uint8_t nameLen = m_Name.size();
                strm.write((char*)(&nameLen), sizeof(nameLen));
                strm.write(m_Name.data(), nameLen);

                for (auto &&p : m_Properties)
                    p->Serialize(strm);

                if(!m_NestedNodes.empty())
                {
                    for (auto &&n : m_NestedNodes)
                        n->Serialize(strm);

                    // Null record
                    for(char c = 0; c < 13; c++)
                        strm << '\x00';
                }
            }

            inline uint32_t Size() const
            {
                return m_Size;
            }

            static FBXNode CreateNestedNode(const std::string &Name)
            {
                auto node = FBXNode(new CFBXNode());
                node->SetName(Name);

                return node;
            }

            template<class T>
            void WriteType()
            {
                static const std::unordered_map<std::type_index, std::vector<std::string>> TYPES = {
                    {typeid(int), {"int", "Integer"}},
                    {typeid(double64), {"double", "Number"}},
                    {typeid(std::string), {"KString", ""}},
                    {typeid(CColor), {"ColorRGB", "Color"}},
                };

                auto types = TYPES.at(typeid(T));
                for (auto &&t : types)
                    AddProperty(FBXProperty(new CFBXProperty<std::string>(t)));
            }

            virtual ~CFBXNode() = default;

        protected:
            uint32_t m_Size;

        private:
            uint32_t m_PropertiesSize;

            std::vector<FBXProperty> m_Properties;
            std::string m_Name;
            std::vector<FBXNode> m_NestedNodes;
    };


    class CFBXCreationTimeStamp : public CFBXNode
    {
        public:
            CFBXCreationTimeStamp() : CFBXNode()
            {
                SetName("CreationTimeStamp");
                auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                struct tm *tm = localtime(&time);

                FBXNode node = CreateNestedNode("Version");
                node->AddProperty(FBXProperty(new CFBXProperty<int>(1000)));
                AddNestedNode(node);

                node = CreateNestedNode("Year");
                node->AddProperty(FBXProperty(new CFBXProperty<int>(tm->tm_year + 1900)));
                AddNestedNode(node);

                node = CreateNestedNode("Month");
                node->AddProperty(FBXProperty(new CFBXProperty<int>(tm->tm_mon + 1)));
                AddNestedNode(node);

                node = CreateNestedNode("Day");
                node->AddProperty(FBXProperty(new CFBXProperty<int>(tm->tm_mday)));
                AddNestedNode(node);

                node = CreateNestedNode("Hour");
                node->AddProperty(FBXProperty(new CFBXProperty<int>(tm->tm_hour)));
                AddNestedNode(node);

                node = CreateNestedNode("Minute");
                node->AddProperty(FBXProperty(new CFBXProperty<int>(tm->tm_min)));
                AddNestedNode(node);

                node = CreateNestedNode("Second");
                node->AddProperty(FBXProperty(new CFBXProperty<int>(tm->tm_sec)));
                AddNestedNode(node);

                node = CreateNestedNode("Millisecond");
                node->AddProperty(FBXProperty(new CFBXProperty<int>(0)));
                AddNestedNode(node);
            }
    };

    class CFBXHeaderExtension : public CFBXNode
    {
        public:
            CFBXHeaderExtension() : CFBXNode()
            {
                SetName("FBXHeaderExtension");

                FBXNode node = CreateNestedNode("FBXHeaderVersion");
                node->AddProperty(FBXProperty(new CFBXProperty<int>(1003)));
                AddNestedNode(node);

                node = CreateNestedNode("FBXVersion");
                node->AddProperty(FBXProperty(new CFBXProperty<int>(7400)));
                AddNestedNode(node);

                node = CreateNestedNode("Creator");
                node->AddProperty(FBXProperty(new CFBXProperty<std::string>("Generated with VoxelOptimizer")));
                AddNestedNode(node);

                auto creationTime = FBXNode(new CFBXCreationTimeStamp());
                AddNestedNode(creationTime);
            }
    };

    class CGlobalSettings : public CFBXNode
    {
        public:
            CGlobalSettings() : CFBXNode()
            {
                SetName("GlobalSettings");

                FBXNode node = CreateNestedNode("Version");
                node->AddProperty(FBXProperty(new CFBXProperty<int>(1000)));
                AddNestedNode(node);

                node = CreateNestedNode("Properties70");
                auto prop = node->CreateNestedNode("P");
                prop->AddProperty(FBXProperty(new CFBXProperty<std::string>("UpAxis")));
                prop->WriteType<int>();
                prop->AddProperty(FBXProperty(new CFBXProperty<int>(1)));

                prop = node->CreateNestedNode("P");
                prop->AddProperty(FBXProperty(new CFBXProperty<std::string>("UpAxisSign")));
                prop->WriteType<int>();
                prop->AddProperty(FBXProperty(new CFBXProperty<int>(1)));

                prop = node->CreateNestedNode("P");
                prop->AddProperty(FBXProperty(new CFBXProperty<std::string>("FrontAxis")));
                prop->WriteType<int>();
                prop->AddProperty(FBXProperty(new CFBXProperty<int>(2)));

                prop = node->CreateNestedNode("P");
                prop->AddProperty(FBXProperty(new CFBXProperty<std::string>("FrontAxisSign")));
                prop->WriteType<int>();
                prop->AddProperty(FBXProperty(new CFBXProperty<int>(1)));

                prop = node->CreateNestedNode("P");
                prop->AddProperty(FBXProperty(new CFBXProperty<std::string>("CoordAxis")));
                prop->WriteType<int>();
                prop->AddProperty(FBXProperty(new CFBXProperty<int>(0)));

                prop = node->CreateNestedNode("P");
                prop->AddProperty(FBXProperty(new CFBXProperty<std::string>("CoordAxisSign")));
                prop->WriteType<int>();
                prop->AddProperty(FBXProperty(new CFBXProperty<int>(1)));
                
                prop = node->CreateNestedNode("P");
                prop->AddProperty(FBXProperty(new CFBXProperty<std::string>("OriginalUpAxis")));
                prop->WriteType<int>();
                prop->AddProperty(FBXProperty(new CFBXProperty<int>(-1)));

                prop = node->CreateNestedNode("P");
                prop->AddProperty(FBXProperty(new CFBXProperty<std::string>("OriginalUpAxisSign")));
                prop->WriteType<int>();
                prop->AddProperty(FBXProperty(new CFBXProperty<int>(1)));

                prop = node->CreateNestedNode("P");
                prop->AddProperty(FBXProperty(new CFBXProperty<std::string>("UnitScaleFactor")));
                prop->WriteType<double64>();
                prop->AddProperty(FBXProperty(new CFBXProperty<double64>(1)));

                prop = node->CreateNestedNode("P");
                prop->AddProperty(FBXProperty(new CFBXProperty<std::string>("OriginalUnitScaleFactor")));
                prop->WriteType<double64>();
                prop->AddProperty(FBXProperty(new CFBXProperty<double64>(1)));

                prop = node->CreateNestedNode("P");
                prop->AddProperty(FBXProperty(new CFBXProperty<std::string>("AmbientColor")));
                prop->WriteType<CColor>();
                prop->AddProperty(FBXProperty(new CFBXProperty<CColor>(CColor(0, 0, 0, 0))));

                prop = node->CreateNestedNode("P");
                prop->AddProperty(FBXProperty(new CFBXProperty<std::string>("DefaultCamera")));
                prop->WriteType<std::string>();
                prop->AddProperty(FBXProperty(new CFBXProperty<std::string>("Producer Perspective")));

                AddNestedNode(node);
            }
    };

    class CDocuments : public CFBXNode
    {
        public:
            CDocuments() : CFBXNode()
            {
                SetName("Documents");
                auto node = CreateNestedNode("Count");
                node->AddProperty(FBXProperty(new CFBXProperty<int>(1)));
                AddNestedNode(node);

                node = CreateNestedNode("Document");
                node->AddProperty(FBXProperty(new CFBXProperty<int64_t>(UUID)));
                node->AddProperty(FBXProperty(new CFBXProperty<std::string>("VoxelOptimizerScene")));
                node->AddProperty(FBXProperty(new CFBXProperty<std::string>("VoxelOptimizerScene")));
                AddNestedNode(node);

                node = CreateNestedNode("Properties70");
                auto prop = CreateNestedNode("P");
                prop->AddProperty(FBXProperty(new CFBXProperty<std::string>("SourceObject")));
                prop->AddProperty(FBXProperty(new CFBXProperty<std::string>("object")));
                prop->AddProperty(FBXProperty(new CFBXProperty<std::string>("")));
                prop->AddProperty(FBXProperty(new CFBXProperty<std::string>("")));
                node->AddNestedNode(prop);

                prop = CreateNestedNode("P");
                prop->AddProperty(FBXProperty(new CFBXProperty<std::string>("ActiveAnimStackName")));
                prop->WriteType<std::string>();
                node->AddNestedNode(prop);
                AddNestedNode(node);

                node = CreateNestedNode("RootNode");
                node->AddProperty(FBXProperty(new CFBXProperty<int64_t>(0)));
                AddNestedNode(node);
            }
    };
} // namespace VoxelOptimizer


#endif //FBXNODES_HPP