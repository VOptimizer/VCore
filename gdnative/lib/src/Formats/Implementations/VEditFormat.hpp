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

#ifndef VEDITFORMAT_HPP
#define VEDITFORMAT_HPP

#include <string>
#include <string.h>
#include <VoxelOptimizer/Misc/BinaryStream.hpp>
#include <VoxelOptimizer/Formats/IVoxelFormat.hpp>
#include <VoxelOptimizer/Meshing/Material.hpp>

namespace VoxelOptimizer
{
    enum SectionType
    {
        META,
        MATERIAL,
        COLORPALETTE,
        VOXELS,
        SCENE_TREE
    };

    enum AnyType : uint8_t
    {
        STRING,
        FLOAT,
        INT32,
        UINT32,
        VECTOR3
    };

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

    class CVEditFormat : public IVoxelFormat
    {
        public:
            CVEditFormat() = default;

            std::vector<char> Save(const std::vector<VoxelMesh> &meshes) override;

            ~CVEditFormat() = default;

        protected:
            void ParseFormat() override;
    };
} // namespace VoxelOptimizer

#endif //VEDITFORMAT_HPP