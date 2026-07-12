#include <memory>
#include <iostream>

#include "VoxelEngine/utils/version.hpp"
#include "VoxelEngine/Game/Game.hpp"
#include "VoxelEngine/Engine/Engine.hpp"

VoxelEngine::Version version(0u, 1u, 0u);

class Demo : public VoxelEngine::Game {
    public:
        Demo() : VoxelEngine::Game("Demo", version) {};

        void Init(VoxelEngine::EngineService& service) override {
            service.setVSync(true);
            std::cout << version.toString() << std::endl;
        }

        void Update() override {}
        void Render() override {}
        void Shutdown() override {}
};

int main() {
    std::unique_ptr<Demo> demo = std::make_unique<Demo>();
    VoxelEngine::Engine engine(std::move(demo));

    engine.run();

    return EXIT_SUCCESS;
}
