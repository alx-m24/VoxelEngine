#pragma once

#include "VoxelEngine/Maths/units.hpp"

namespace VoxelEngine {
    class Camera {
        public:
            Camera() = default;
            virtual ~Camera() = default;

        public:
            const mat4& getCameraMatrix() const { return m_cameraMatrix; }

        protected:
            virtual void RecalculateCameraMatrix() = 0;

        protected:
            mat4 m_cameraMatrix{ 1.0f };
    };
}
