#include "Camera.hpp"

Camera::Camera(GLFWwindow* window, Shader& shader)
{
    useCam = true;
    shader.use();
    this->update(window, shader, 0.0f);
    useCam = false;
}

void Camera::update(GLFWwindow* window, Shader& shader, float dt)
{
    if (useCam) {
        this->Yaw += xoffset * mouseSens;
        this->Pitch += yoffset * mouseSens;

        if (Pitch < -89.0f) Pitch = -89.0f;
        else if (Pitch > 89.0f) Pitch = 89.0f;

        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front = glm::normalize(front);
        right = glm::normalize(glm::cross(front, WorldUp));

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            this->move(Camera::FORWARD, dt);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            this->move(Camera::BACKWARD, dt);
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            this->move(Camera::LEFT, dt);
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            this->move(Camera::RIGHT, dt);
    }

    viewMatrix = glm::lookAt(Position, Position + front, WorldUp);

    shader.setVec2("uResolution", SCR_WIDTH, SCR_HEIGHT);
    shader.setMat3("uViewMatrix", viewMatrix);
    shader.setVec3("uOrigin", Position);
    shader.setFloat("maxDist", far);
    shader.setFloat("fov", glm::radians(FOV));
}

void Camera::move(Movement movement, float dt)
{
    float velocity = MovementSpeed * dt;

    if (movement == FORWARD) Position += front * velocity;
    if (movement == BACKWARD) Position -= front * velocity;
    if (movement == LEFT) Position -= right * velocity;
    if (movement == RIGHT) Position += right * velocity;
}
