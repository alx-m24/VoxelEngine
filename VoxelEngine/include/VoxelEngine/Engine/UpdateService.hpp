#pragma once
#include "VoxelEngine/Camera/Camera.hpp"

namespace VoxelEngine {
    class UpdateService {
        protected:
            const Camera* activeCamera{};
            float          deltaTime{};

        public:
            UpdateService() = default;
            virtual ~UpdateService() = default;

            virtual void setActiveCamera(const Camera* cam) = 0;
            const Camera* getActiveCamera() const { return activeCamera; }

            float getDeltaTime() const   { return deltaTime; }
    };
}
