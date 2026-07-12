#pragma once

#include <cstdint>
#include <string>

namespace VoxelEngine {
    class Version {
        private:
            const uint8_t m_major{}, m_minor{}, m_patch{};

        public: 
            Version(uint8_t Major, uint8_t Minor, uint8_t Patch) : m_major(Major), m_minor(Minor), m_patch(Patch) {}

        public:
            std::string toString() const {
                return "v" + std::to_string(m_major) + "." + std::to_string(m_minor) + "." + std::to_string(m_patch);
            }
    };
}
