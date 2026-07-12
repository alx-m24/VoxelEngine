#include "VoxelEngine/Engine/Engine.hpp"

#include "VoxelEngine/utils/assert.hpp"

namespace VoxelEngine {
    void Engine::Init() {
        V_ASSERT(m_game, "Game cannod be nullptr");

        m_window.Init(m_game->getName());
    }

    void Engine::run() {
        this->Init();

        m_game->Init(*this);

        while (!m_window.shouldClose()) {
            glfwPollEvents();

            glClear(GL_COLOR_BUFFER_BIT);
            glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

            m_window.swapBuffers();
        }
    }

    void Engine::RequestClose() {
    }

    void Engine::setCursorMode(EngineService::CURSOR_MODE mode) {
        m_window.setCursorMode(mode == EngineService::CURSOR_MODE::LOCKED ? 
                Window::CURSOR_MODE::LOCKED : Window::CURSOR_MODE::FREE);
    }

    void Engine::setVSync(bool on) {
        glfwSwapInterval(static_cast<int>(on));
    }
}
