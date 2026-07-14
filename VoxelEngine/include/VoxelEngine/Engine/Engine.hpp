#pragma once

#include <memory>

#include "VoxelEngine/Engine/EngineService.hpp"
#include "VoxelEngine/Window/Window.hpp"
#include "VoxelEngine/Game/Game.hpp"

namespace VoxelEngine {
    class Engine : public EngineService {
        private:
            std::unique_ptr<Game> m_game{};
            Window m_window{};

        public:
            Engine(const char* name) { this->Init(name); }

        public:
            void setGame(std::unique_ptr<Game>&& game) { m_game = std::move(game); }
            void run();

            void RequestClose() override;
            void setCursorMode(EngineService::CURSOR_MODE mode) override;
            void setVSync(bool on) override;

        private:
            void Init(const char* name);
    };
}
