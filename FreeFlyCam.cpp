//
// Created by maciej on 28.07.25.
//

#include "FreeFlyCam.hpp"

#include <math.h>
#include <GLFW/glfw3.h>
#include <glm/trigonometric.hpp>
#include <glm/detail/func_trigonometric.inl>
#include <glm/ext/quaternion_geometric.hpp>

void FreeFlyCam::processKeyboard(bool *keys, float deltaTime) {
    float velocity = moveSpeed * deltaTime;

    if (keys[GLFW_KEY_W])
        position += front * velocity;
    if (keys[GLFW_KEY_S])
        position -= front * velocity;
    if (keys[GLFW_KEY_A])
        position -= right * velocity;
    if (keys[GLFW_KEY_D])
        position += right * velocity;
    if (keys[GLFW_KEY_SPACE])
        position += up * velocity;
    if (keys[GLFW_KEY_LEFT_SHIFT])
        position -= up * velocity;
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


void FreeFlyCam::updateVectors()
{
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw) * cos(glm::radians(pitch)));

    front = glm::normalize(newFront);
    right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::normalize(glm::cross(right, front));
}
