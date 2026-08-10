#pragma once

#include <cstdint>
#include <string>

namespace VoxelEngine {
    class Version {
        public:
            Version(uint32_t major, uint32_t minor, uint32_t hotfix) : m_major(major), m_minor(minor), m_hotfix(hotfix) {}
    
            std::string toString() const { 
                return "v" + std::to_string(m_major) + '.' + std::to_string(m_minor) + '.' + std::to_string(m_hotfix);
            }
    
        private:
            uint32_t m_major;
            uint32_t m_minor;
            uint32_t m_hotfix;
    };

}
