#include "pch.h"
#include "VoxelEngine/Camera/Derived/Perspective.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace VoxelEngine {
    PerspectiveCamera::PerspectiveCamera(vec3 position, float fovDegrees, float aspect, float nearPlane, float farPlane)
        : m_position(position), m_fov(fovDegrees), m_aspect(aspect), m_near(nearPlane), m_far(farPlane)
    {
        RecalculateVectors();
        RecalculateCameraMatrix();
    }

    void PerspectiveCamera::setPosition(const vec3& position) {
        m_position = position;
        RecalculateCameraMatrix();
    }

    void PerspectiveCamera::setRotation(float yawDegrees, float pitchDegrees) {
        m_yaw = yawDegrees;
        m_pitch = glm::clamp(pitchDegrees, -89.0f, 89.0f);
        RecalculateVectors();
        RecalculateCameraMatrix();
    }

    void PerspectiveCamera::setFov(float fovDegrees) {
        m_fov = fovDegrees;
        RecalculateCameraMatrix();
    }

    void PerspectiveCamera::setAspect(float aspect) {
        m_aspect = aspect;
        RecalculateCameraMatrix();
    }

    void PerspectiveCamera::setClipPlanes(float nearPlane, float farPlane) {
        m_near = nearPlane;
        m_far = farPlane;
        RecalculateCameraMatrix();
    }

    void PerspectiveCamera::RecalculateVectors() {
        vec3 front;
        front.x = glm::cos(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch));
        front.y = glm::sin(glm::radians(m_pitch));
        front.z = glm::sin(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch));

        m_front = glm::normalize(front);
        m_right = glm::normalize(glm::cross(m_front, vec3(0.0f, 1.0f, 0.0f)));
        m_up    = glm::normalize(glm::cross(m_right, m_front));
    }

    void PerspectiveCamera::RecalculateCameraMatrix() {
        mat4 view = glm::lookAt(m_position, m_position + m_front, m_up);
        mat4 projection = glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far);
        m_cameraMatrix = projection * view;
    }
}
