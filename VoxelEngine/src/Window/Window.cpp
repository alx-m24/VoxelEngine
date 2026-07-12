#include "pch.h"
#include "VoxelEngine/Window/Window.hpp"
#include "VoxelEngine/utils/assert.hpp"
#include "VoxelEngine/Input/Input.hpp"
#include "VoxelEngine/Maths/vec.hpp"

#include <imgui.h>
#include <implot.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace VoxelEngine {
    constexpr uvec2 DEFAULT_WINDOW_SIZE = { 1000, 800 };

    void Window::FrameBufferSizeCallback(GLFWwindow* glfw_window, int width, int height) {
        Window* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
        window->size = { width, height };
        glViewport(0, 0, width, height);
    }

    Window::~Window() {
        if (m_window) Shutdown();
    }

    void Window::setCursorMode(CURSOR_MODE mode) const {
        glfwSetInputMode(m_window,
                GLFW_CURSOR,
                mode == CURSOR_MODE::LOCKED ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL
            );
    }

    void Window::swapBuffers() const {
        glfwSwapBuffers(m_window);
    }
            
    void Window::Init(std::string_view windowName) {
        glfwInit(); // internal does NOT reset if already called (safe)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_window = glfwCreateWindow(
                static_cast<int>(DEFAULT_WINDOW_SIZE.x),
                static_cast<int>(DEFAULT_WINDOW_SIZE.y),
                std::string(windowName).c_str(),
                nullptr,
                nullptr
            );
        V_ASSERT(m_window != nullptr, "Failed to create window");

        this->size = DEFAULT_WINDOW_SIZE;

        glfwMakeContextCurrent(m_window);

	    glfwSetWindowUserPointer(m_window, this);
	    glfwSetKeyCallback(m_window, Input::keyCallback);
	    glfwSetCursorPosCallback(m_window, Input::MousePositionCallback);
	    glfwSetMouseButtonCallback(m_window, Input::MouseButtonCallback);
	    glfwSetScrollCallback(m_window, Input::MouseScrollCallback);
	    glfwSetFramebufferSizeCallback(m_window, Window::FrameBufferSizeCallback);

        V_ASSERT(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to initialize GLAD");

	    IMGUI_CHECKVERSION();
	    ImGui::CreateContext();

	    ImGuiIO& io = ImGui::GetIO(); (void)io;
	    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	    ImGui::StyleColorsDark();

	    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
	    ImGui_ImplOpenGL3_Init("#version 460");

	    ImPlot::CreateContext();
    }

    bool Window::shouldClose() const {
        return glfwWindowShouldClose(m_window);
    }

    void Window::Shutdown() {
        if (m_window) glfwDestroyWindow(m_window); 
    }
}
