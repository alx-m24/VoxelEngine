#pragma once

#include <string_view>

#include "VoxelEngine/Engine/EngineService.hpp"
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
            virtual void Init(EngineService& service) = 0;
            virtual void Update() = 0;
            virtual void Render() = 0;
            virtual void Shutdown() = 0;

        public:
            std::string_view getName() const { return m_name; }
            Version getVersion() const { return m_version; }
    };
}
