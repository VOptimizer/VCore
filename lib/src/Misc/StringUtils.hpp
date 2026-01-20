#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include "VCore/Misc/fast_vector.hpp"
#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <VCore/Math/Vector.hpp>

namespace VCore 
{
    template<class T>
    inline T ConvertRangeToNumber(std::string_view::const_iterator p_Begin, std::string_view::const_iterator p_End)
    {
        int result = 0;
        bool negative = false;
        while (p_Begin != p_End) 
        {
            if(*p_Begin == '-')
                negative = true;
            else
                result = result * 10 + (*p_Begin - '0');

            p_Begin++;
        }

        return result * (negative ? -1 : 1);
    }

    template<>
    inline float ConvertRangeToNumber<float>(std::string_view::const_iterator p_Begin, std::string_view::const_iterator p_End)
    {
        float result = 0;
        int mantissa = 0;
        bool negative = false, isMantissa = false;
        float divider = 1;

        while (p_Begin != p_End) 
        {
            if(*p_Begin == '-')
                negative = true;
            if(*p_Begin == '.')
                isMantissa = true;
            else
            {
                if(isMantissa)
                {
                    mantissa = mantissa * 10 + (*p_Begin - '0');
                    divider *= 10;
                }
                else
                    result = result * 10 + (*p_Begin - '0');
            }

            p_Begin++;
        }

        return (result + mantissa / divider) * (negative ? -1 : 1);
    }

    template<class T>
    inline Math::TVector3<T> ParseVector(std::string_view p_View)
    {
        Math::TVector3<T> result;
        auto spaceIt = std::find(p_View.begin(), p_View.end(), ' ');

        result.x = ConvertRangeToNumber<T>(p_View.begin(), spaceIt);

        auto begin = spaceIt + 1;
        spaceIt = std::find(begin, p_View.end(), ' ');
        result.y = ConvertRangeToNumber<T>(begin, spaceIt);

        begin = spaceIt + 1;
        spaceIt = std::find(begin, p_View.end(), ' ');
        result.z = ConvertRangeToNumber<T>(begin, spaceIt);

        return result;
    }

    inline fast_vector<std::string> Split(std::string_view p_View, char p_Delimiter, int64_t p_SplitAfterOccurences = -1)
    {
        fast_vector<std::string> result;
        std::string buffer;

        for (auto &&c : p_View) 
        {
            if(p_SplitAfterOccurences > 0 && c == p_Delimiter)
            {
                p_SplitAfterOccurences--;
                buffer += c;
            }
            else if(c == p_Delimiter)
            {
                result.push_back(buffer);
                buffer.clear();
            }
            else
                buffer += c;
        }

        if(!buffer.empty())
            result.push_back(buffer);

        return result;
    }
}

#endif