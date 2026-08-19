#pragma once

#include <string_view>
#include <functional>
#include <vector>

#include "VoxelEngine/Maths/vec.hpp"

// Forward delcaration
struct GLFWwindow;

namespace VoxelEngine {
    class Window {
        private:
            GLFWwindow* m_window = nullptr;
            uvec2 size;
            
        public:
            using ResizeCallback_T = std::function<void(uint32_t, uint32_t)>;

        private:
            std::vector<ResizeCallback_T> m_resizeCallbacks;

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

        public:
            template<typename T>
            void addResizeCallback(T&& callback) {
                m_resizeCallbacks.emplace_back(std::forward<T&&>(callback));
            }

        private:
            static void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
    };
}
