#ifndef VOXELTESTFORMAT_HPP
#define VOXELTESTFORMAT_HPP

#include <VCore/VCore.hpp>
#include "../lib/src/FileUtils.hpp"
#include <VCore/Math/Vector.hpp>
#include <VCore/Misc/unordered_dense.h>
#include <VCore/Voxel/BBox.hpp>
#include <VCore/Voxel/Voxel.hpp>
#include <cstdint>
#include <fstream>
#include <ios>
#include <memory>
#include <string>
#include "StringMakerSpecializations.hpp"
#include "VCore/Misc/fast_vector.hpp"

class CVoxelTestFormat
{
    public:
        CVoxelTestFormat(const std::string &p_Path) : m_File(VCore::GetPathWithoutExt(p_Path) + ".txt", std::ios::binary | std::ios::in) 
        {
            ParseFile();
        }

        void ValidateModel(const VCore::VoxelModel &p_Model)
        {
            CHECK_EQ(p_Model->Size(), m_VoxelCount);

            auto bbox = p_Model->CalculateBBox();

            CHECK_EQ(bbox.Beg, m_BBox.Beg);
            CHECK_EQ(bbox.End, m_BBox.End);
            CHECK_EQ(bbox.GetSize(), m_BBox.GetSize());

            for (auto &voxel : *p_Model) 
            {
                auto it = m_Voxels.find(voxel.first);
                REQUIRE_NE(it, m_Voxels.end());
                CHECK_EQ(it->second, voxel.second);
            }
        }
    private:
        void ParseFile()
        {
            m_File >> m_VoxelCount;
            m_File.get();

            m_File >> m_BBox.Beg.x >> m_BBox.Beg.y >> m_BBox.Beg.z >> m_BBox.End.x >> m_BBox.End.y >> m_BBox.End.z;
            m_File.get();

            while (m_File.good()) 
            {
                VCore::Math::Vec3i pos;
                uint32_t color, mat;
                m_File >> pos.x >> pos.y >> pos.z >> color >> mat;
                m_File.get();

                m_Voxels.insert({pos, VCore::CVoxel(color, mat)});
            }
        }

        VCore::CBBox m_BBox;
        uint64_t m_VoxelCount;
        ankerl::unordered_dense::map<VCore::Math::Vec3i, VCore::CVoxel, VCore::Math::Vec3iHasher> m_Voxels;

        std::ifstream m_File;
};

#endif