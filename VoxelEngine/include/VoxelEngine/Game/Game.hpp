#pragma once

#include "VoxelEngine/Engine/EngineService.hpp"
#include "VoxelEngine/utils/version.hpp"

namespace VoxelEngine {
    class Game {
        private:
            const Version m_version;

        public:
            Game(const Version& version) : m_version(version) {}
            virtual ~Game() = default;

        public:
            virtual void Init(EngineService& service) = 0;
            virtual void Update() = 0;
            virtual void Render() = 0;
            virtual void Shutdown() = 0;

        public:
            Version getVersion() const { return m_version; }
    };
}
