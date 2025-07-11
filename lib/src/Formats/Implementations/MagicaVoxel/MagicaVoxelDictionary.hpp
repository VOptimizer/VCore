#ifndef MAGICAVOXELDICTIONARY_HPP
#define MAGICAVOXELDICTIONARY_HPP

#include <VCore/Misc/FileStream.hpp>

namespace VCore 
{
    class CMagicaVoxelDictionary
    {
        public:
            CMagicaVoxelDictionary(IFileStream *p_DataStream) : m_DataStream(p_DataStream) {}

            /**
             * @brief Writes a string.
             * A string is stored without null termination and has an int32 as "prefix" which contains the size of the string.
             */
            inline void WriteString(const char *p_String)
            {
                uint32_t size = strlen(p_String);

                // Size "prefix"
                m_DataStream->Write(size);

                // String without null termination.
                m_DataStream->Write(p_String, size);
            }

            inline void WriteDictKeyString(const char *p_Key, const char *p_Value)
            {
                WriteString(p_Key);
                WriteString(p_Value);
            }

            inline void WriteDictKeyInt(const char *p_Key, const int p_Value)
            {
                WriteString(p_Key);

                // Formats the float number to a string with two digits after the decimal point.
                uint32_t size = snprintf(nullptr, 0, "%i", p_Value);
                char *value = new char[size + 1];
                snprintf(value, size + 1, "%i", p_Value);
                
                m_DataStream->Write(size);
                m_DataStream->Write(value, size);
                delete[] value;
            }

            inline void WriteDictKeyFloat(const char *p_Key, const float p_Value)
            {
                WriteString(p_Key);

                // Formats the float number to a string with two digits after the decimal point.
                uint32_t size = snprintf(nullptr, 0, "%.2f", p_Value);
                char *value = new char[size + 1];
                snprintf(value, size + 1, "%.2f", p_Value);
                
                m_DataStream->Write(size);
                m_DataStream->Write(value, size);
                delete[] value;
            }

            inline void WriteDictKeyVec3i(const char *p_Key, const Math::Vec3i &p_Value)
            {
                WriteString(p_Key);

                // Formats a Vec3i in the format "X Z Y", since MagicaVoxel uses the Z Axis as gravity axis.
                uint32_t size = snprintf(nullptr, 0, "%i %i %i", p_Value.x, p_Value.z, p_Value.y);
                char *value = new char[size + 1];
                snprintf(value, size + 1, "%i %i %i", p_Value.x, p_Value.z, p_Value.y);
                
                m_DataStream->Write(size);
                m_DataStream->Write(value, size);
                delete[] value;
            }

            inline void SkipDict()
            {
                uint32_t keys = m_DataStream->Read<uint32_t>();
                for (uint32_t i = 0; i < keys; i++)
                {
                    uint32_t size = m_DataStream->Read<uint32_t>();
                    m_DataStream->Seek(size);
                    size = m_DataStream->Read<uint32_t>();
                    m_DataStream->Seek(size);
                }
            }
        private:
            IFileStream *m_DataStream;
    };
}

#endif