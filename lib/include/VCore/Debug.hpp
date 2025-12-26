#ifndef DEBUG_H
#define DEBUG_H

#include <cstdint>
#include <string>
#include <utility>
namespace VCore
{
    enum class TimeUnit
    {
        NANO,
        MILLI
    };
};

#ifdef DEBUG

#include <chrono>
#include <cstdio>
#include <string_view>
#include <VCore/Misc/unordered_dense.h>

#define START_PROFILER(name, unit) {VCore::CProfiler _profiler(name, unit);
#define END_PROFILER() }

namespace VCore
{
    class CProfiler
    {
        public:
            CProfiler(std::string_view p_FunctionName, TimeUnit p_Unit) : m_FunctionName(p_FunctionName), m_Start(std::chrono::high_resolution_clock::now()), m_Unit(p_Unit)
            {
                if(!s_Instance)
                    s_Instance = this;
            }

            ~CProfiler() 
            {
                auto end = std::chrono::high_resolution_clock::now();
                auto unit = GetUnitStr();
                auto duration = GetDuration(end);
                if(s_Instance == this)
                {
                    printf("%s: %lu%s\n", m_FunctionName.data(), duration, unit);
                    for (auto &&section : m_Sections) 
                        printf("\t%s: Total: %lu%s Called: %lu Avg: %f%s\n", section.first.data(), section.second.first, unit, section.second.second, static_cast<double>(section.second.first) / static_cast<double>(section.second.second), unit);
                    s_Instance = nullptr;
                }
                else
                    s_Instance->UpdateSection(m_FunctionName, duration);
            }
        private:
            uint64_t GetDuration(std::chrono::system_clock::time_point p_End) const 
            {
                if(m_Unit == TimeUnit::MILLI)
                    return std::chrono::duration_cast<std::chrono::milliseconds>(p_End - m_Start).count();

                return std::chrono::duration_cast<std::chrono::nanoseconds>(p_End - m_Start).count();
            }

            const char *GetUnitStr() const
            {
                if(m_Unit == TimeUnit::MILLI)
                    return "ms";

                return "ns";
            }

            void UpdateSection(std::string_view p_Section, uint64_t p_Duration)
            {
                auto sectionName = std::string(p_Section);
                auto it = m_Sections.find(sectionName);
                if(it != m_Sections.end())
                {
                    it->second.first += p_Duration;
                    it->second.second++;
                }
                else
                    m_Sections[sectionName] = std::make_pair(p_Duration, 1);
            }

            static CProfiler *s_Instance;

            std::string_view m_FunctionName;
            std::chrono::system_clock::time_point m_Start;
            ankerl::unordered_dense::map<std::string, std::pair<uint64_t, uint64_t>> m_Sections;
            TimeUnit m_Unit;
    };

    inline CProfiler *CProfiler::s_Instance = nullptr;
}

#else
#define START_PROFILER(name, unit)
#define END_PROFILER()
#endif

#endif