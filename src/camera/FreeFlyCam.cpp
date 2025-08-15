//
// Created by maciej on 28.07.25.
//

#include "FreeFlyCam.hpp"

#include <cmath>
#include <GLFW/glfw3.h>
#include <glm/trigonometric.hpp>
//#include <glm/detail/func_trigonometric.inl>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_geometric.hpp>

FreeFlyCam::FreeFlyCam(glm::vec3 startPos)
    : position(startPos),
        front(0.0f, 0.0f, -1.0f),
        up(0.0f, 1.0f, 0.0f),
        right(0.0f, 0.0f, 0.0f),
        yaw(-90.0f),
        pitch(0.0f),
        moveSpeed(15.0f),
        mouseSensitivity(0.1f)
{
 updateVectors();
}


void FreeFlyCam::processKeyboard(const bool *keys, float deltaTime) {
    float velocity = moveSpeed * deltaTime;

    moveSpeed = (keys[GLFW_KEY_LEFT_CONTROL] ? 60.0f : 15.0f);
    if (keys[GLFW_KEY_W])
        position += front * velocity;
    if (keys[GLFW_KEY_S])
        position -= front * velocity;
    if (keys[GLFW_KEY_A])
        position -= right * velocity;
    if (keys[GLFW_KEY_D])
        position += right * velocity;
    if (keys[GLFW_KEY_SPACE])
        position += glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
    if (keys[GLFW_KEY_LEFT_SHIFT])
        position -= glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
}

void FreeFlyCam::processMouse(float xOffset, float yOffset) {
    xOffset *= mouseSensitivity;
    yOffset *= mouseSensitivity;

    yaw += xOffset;
    pitch += yOffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    updateVectors();
}

glm::mat4 FreeFlyCam::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

void FreeFlyCam::updateVectors()
{
    glm::vec3 newFront;
    newFront.x = static_cast<float>(cos(glm::radians(yaw)) * cos(glm::radians(pitch)));
    newFront.y = static_cast<float>(sin(glm::radians(pitch)));
    newFront.z = static_cast<float>(sin(glm::radians(yaw)) * cos(glm::radians(pitch)));
    this->front = glm::normalize(newFront);

    front = glm::normalize(newFront);
    right = glm::normalize(glm::cross(this->front, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::vec3(0, 1, 0);
}
