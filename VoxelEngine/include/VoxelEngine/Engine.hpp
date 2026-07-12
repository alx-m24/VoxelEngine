#pragma once

#include <memory>

#include "VoxelEngine/Game/Game.hpp"

namespace VoxelEngine {
    class Engine {
        private:
            std::unique_ptr<Game> m_game{};

        public:
            Engine(std::unique_ptr<Game> game) : m_game(std::move(game)) {}

        public:
            void run();
    };
}
