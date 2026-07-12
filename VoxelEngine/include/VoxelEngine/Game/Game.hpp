#pragma once

#include <string_view>

#include "VoxelEngine/utils/version.hpp"

namespace VoxelEngine {
    class Game {
        private:
            const std::string_view m_name; 
            const Version m_version;

        public:
            Game(std::string_view name, const Version& version) : m_name(name), m_version(version) {}
            virtual ~Game() = default;

        public:
            virtual void Init() = 0;
            virtual void Update() = 0;
            virtual void Render() = 0;
            virtual void Shutdown() = 0;
    };
}
