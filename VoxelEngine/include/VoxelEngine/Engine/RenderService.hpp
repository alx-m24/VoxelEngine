#pragma once
#include "VoxelEngine/Maths/vec.hpp"

namespace VoxelEngine {
    class RenderService {
        protected:
            uvec2          viewportSize{};

        public:
            RenderService() = default;
            virtual ~RenderService() = default;

            uvec2 getViewportSize() const { return viewportSize; }

            float getAspectRatio() const {
                return viewportSize.y != 0
                    ? static_cast<float>(viewportSize.x) / static_cast<float>(viewportSize.y)
                    : 0.0f;
            }
    };
}
