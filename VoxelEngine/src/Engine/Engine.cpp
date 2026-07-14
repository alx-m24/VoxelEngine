#include "VoxelEngine/Engine/Engine.hpp"

#include "VoxelEngine/utils/assert.hpp"

namespace VoxelEngine {
    void Engine::Init(const char* name) {
        m_window.Init(name);
    }

    void Engine::run() {
        m_game->Init(*this);

        while (!m_window.shouldClose()) {
            glfwPollEvents();

            m_game->Update();

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glViewport(0.0, 0.0, m_window.size.x, m_window.size.y);

            m_game->Render();

            m_window.swapBuffers();
        }

        m_game->Shutdown();
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
