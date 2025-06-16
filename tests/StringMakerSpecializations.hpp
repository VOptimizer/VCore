#ifndef STRINGMAKERSPECIALIZATIONS_HPP
#define STRINGMAKERSPECIALIZATIONS_HPP

#include "VCore/Math/Vector.hpp"
#include "doctest.h"
#include <VCore/VCore.hpp>
#include <format>

namespace doctest 
{
    template<> 
    struct StringMaker<VCore::CVoxel> 
    {
        static String convert(const VCore::CVoxel& p_Voxel) 
        {
            return std::format("Voxel(Color: {}, Material: {})", p_Voxel.GetColor(), p_Voxel.GetMaterial()).c_str();
        }
    };

    template<> 
    struct StringMaker<VCore::Math::Vec3i> 
    {
        static String convert(const VCore::Math::Vec3i& p_Vec) 
        {
            return std::format("Vec3i({}, {}, {})", p_Vec.x, p_Vec.y, p_Vec.z).c_str();
        }
    };
}

#endif