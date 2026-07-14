#pragma once

#include <cstdint>
#include <string_view>

#include "VoxelEngine/Maths/vec.hpp"

// Forward delcaration
struct GLFWwindow;

namespace VoxelEngine {
    class Window {
        private:
            GLFWwindow* m_window = nullptr;
            uvec2 size;

        public:
            Window() = default;
            ~Window();
            
        private:
            friend class Engine;
            void Init(std::string_view windowName);
            void Shutdown();

            enum class CURSOR_MODE : uint8_t {
                FREE = 0,
                LOCKED
            };
            void setCursorMode(CURSOR_MODE mode) const;
            void swapBuffers() const;
            bool shouldClose() const;
            
        private:
            static void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
    };
}
