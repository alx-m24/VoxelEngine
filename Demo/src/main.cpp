#include <memory>
#include <iostream>

#include "VoxelEngine/utils/version.hpp"
#include "VoxelEngine/Game/Game.hpp"
#include "VoxelEngine/Engine/Engine.hpp"
#include "VoxelEngine/Bindable/SmartBinder.hpp"
#include "VoxelEngine/Drawable/Primitives/Quad.hpp"
#include "VoxelEngine/Shaders/Shader.hpp"

VoxelEngine::Version version(0u, 1u, 0u);

class Demo : public VoxelEngine::Game {
    private:
        VoxelEngine::Shader m_rainbowShader = VoxelEngine::Shader("Demo/Shaders/RainbowRect.vert", "Demo/Shaders/RainbowRect.frag");
        VoxelEngine::Quad m_demoQuad {};

    public:
        Demo() : VoxelEngine::Game("Demo", version) {};

        void Init(VoxelEngine::EngineService& engine) override {
            engine.setVSync(true);
            // update.setActiveCamera(&someCamera);
            std::cout << version.toString() << std::endl;
        }

        void Update(VoxelEngine::UpdateService& update) override {
            // float dt = update.getDeltaTime();
        }

        void Render(VoxelEngine::RenderService& render) override {
            VoxelEngine::SmartBinder shaderBinder(m_rainbowShader);
            m_demoQuad.Draw();
        }

        void Shutdown() override {}
};

int main() {
    VoxelEngine::Engine engine("VoxelEngine - Demo");
    engine.setGame(std::make_unique<Demo>());

    engine.run();

    return EXIT_SUCCESS;
}
