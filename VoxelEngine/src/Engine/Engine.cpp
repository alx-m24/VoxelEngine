#include "pch.h"
#include "VoxelEngine/Engine/Engine.hpp"

#include "VoxelEngine/Drawable/Primitives/Quad.hpp"
#include "VoxelEngine/Bindable/SmartBinder.hpp"
#include "VoxelEngine/Buffer/FrameBuffer.hpp"
#include "VoxelEngine/Texture/Texture.hpp"
#include "VoxelEngine/Shaders/Shader.hpp"
#include "VoxelEngine/utils/assert.hpp"
#include "VoxelEngine/utils/types.hpp"
#include "VoxelEngine/utils/path.hpp"

namespace fs = std::filesystem;

namespace VoxelEngine {
    void Engine::Init(const char* name) {
        m_window.Init(name);
    }

    void Engine::updateFrameTiming() {
        double now = glfwGetTime();
        UpdateService::deltaTime = static_cast<float>(now - m_lastFrameTime);
        m_lastFrameTime = now;
    }

    void Engine::syncViewportSize() {
        RenderService::viewportSize = m_window.size;
    }

    void Engine::run() {
        m_game->Init(*this);

        const fs::path currentDirPath = get_binary_directory();
        const fs::path shaderPath = currentDirPath / "shaders";

        Quad fullScreenQuad{};

        Shader voxel_Pass1{ 
            (shaderPath / "Primitive" / "FullScreen.vert").string(), 
            (shaderPath / "voxel" / "pass1.frag").string() 
        };
        
        Shader mergeShader{ 
            (shaderPath / "Primitive" / "FullScreen.vert").string(), 
            (shaderPath / "Merge" / "merge.frag").string() 
        };

        Texture rasterColor{ m_window.size.x, m_window.size.y, TextureFormat::RGB8 };
        Texture rasterDepth{ m_window.size.x, m_window.size.y, TextureFormat::R16F };
        Texture rasterDepthBuf{ m_window.size.x, m_window.size.y, TextureFormat::DEPTH24 };

        Framebuffer rasterFrameBuffer; 
        rasterFrameBuffer.AttachColor(rasterColor, 0);
        rasterFrameBuffer.AttachColor(rasterDepth, 1);
        rasterFrameBuffer.AttachDepth(rasterDepthBuf);

        V_ASSERT(rasterFrameBuffer.IsComplete(), "Raster Framebuffer incomplete");

        Texture voxelColor{ m_window.size.x, m_window.size.y, TextureFormat::RGB8 };
        Texture voxelDepth{ m_window.size.x, m_window.size.y, TextureFormat::R16F };
        Texture voxelDepthBuf{ m_window.size.x, m_window.size.y, TextureFormat::DEPTH24 };

        Framebuffer voxelFrameBuffer; 
        voxelFrameBuffer.AttachColor(voxelColor, 0);
        voxelFrameBuffer.AttachColor(voxelDepth, 1);
        voxelFrameBuffer.AttachDepth(voxelDepthBuf);

        V_ASSERT(voxelFrameBuffer.IsComplete(), "Voxel Framebuffer incomplete");

        m_window.addResizeCallback(
            [&](uint32_t x, uint32_t y) -> void { voxelFrameBuffer.Resize(x, y); rasterFrameBuffer.Resize(x, y); }
        );

        Window::FrameBufferSizeCallback(m_window.m_window, 1000, 900);

        while (!m_window.shouldClose()) {
            glfwPollEvents();

            updateFrameTiming();
            syncViewportSize();

            m_game->Update(*this);

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            {
                SmartBinder voxelBind(voxelFrameBuffer);
                {
                    SmartBinder shaderBind(voxel_Pass1);
                    if (UpdateService::activeCamera) {
                        voxel_Pass1.setMat4("inv_CameraMatrix", glm::inverse(UpdateService::activeCamera->getCameraMatrix()));
                    }
                    fullScreenQuad.Draw();
                }
            }
            {
                SmartBinder rasterFrameBind(rasterFrameBuffer);
                m_game->Render(*this);
            }

            {
                SmartBinder shaderBind(mergeShader);

                voxelColor.BindUnit(0);
                voxelDepth.BindUnit(1);
                rasterColor.BindUnit(2);
                rasterDepth.BindUnit(3);

                mergeShader.setInt("u_voxelColor", 0);
                mergeShader.setInt("u_voxelDepth", 1);
                mergeShader.setInt("u_rasterColor", 2);
                mergeShader.setInt("u_rasterDepth", 3);

                fullScreenQuad.Draw();
            }

            m_window.swapBuffers();
        }

        m_game->Shutdown();
    }

    void Engine::setActiveCamera(const Camera* cam) {
        UpdateService::activeCamera = cam;
    }

    void Engine::RequestClose() {
        m_window.Shutdown();
    }

    void Engine::setCursorMode(EngineService::CURSOR_MODE mode) {
        m_window.setCursorMode(mode == EngineService::CURSOR_MODE::LOCKED ? 
                Window::CURSOR_MODE::LOCKED : Window::CURSOR_MODE::FREE);
    }

    void Engine::setVSync(bool on) {
        glfwSwapInterval(static_cast<int>(on));
    }
}
