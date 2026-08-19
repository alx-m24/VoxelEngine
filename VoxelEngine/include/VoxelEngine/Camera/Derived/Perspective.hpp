#pragma once

#include "VoxelEngine/Camera/Camera.hpp"

namespace VoxelEngine {
    class PerspectiveCamera : public Camera {
        public:
            PerspectiveCamera(vec3 position, float fovDegrees, float aspect, float nearPlane, float farPlane);

        public:
            void setPosition(const vec3& position);
            void setRotation(float yawDegrees, float pitchDegrees);
            void setFov(float fovDegrees);
            void setAspect(float aspect);
            void setClipPlanes(float nearPlane, float farPlane);

            const vec3& getPosition() const { return m_position; }
            const vec3& getFront() const    { return m_front; }
            const vec3& getRight() const    { return m_right; }
            const vec3& getUp() const       { return m_up; }
            float getYaw() const            { return m_yaw; }
            float getPitch() const          { return m_pitch; }

        protected:
            void RecalculateCameraMatrix() override;

        private:
            void RecalculateVectors();

        private:
            vec3 m_position{};

            vec3 m_front{ 0.0f, 0.0f, -1.0f };
            vec3 m_right{ 1.0f, 0.0f, 0.0f };
            vec3 m_up{ 0.0f, 1.0f, 0.0f };

            // Yaw defaults to -90 so m_front starts pointing down -Z (OpenGL convention)
            float m_yaw = -90.0f;
            float m_pitch = 0.0f;

            float m_fov;
            float m_aspect;
            float m_near;
            float m_far;
    };
}
