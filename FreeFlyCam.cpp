//
// Created by maciej on 28.07.25.
//

#include "FreeFlyCam.hpp"

#include <math.h>
#include <glm/trigonometric.hpp>
#include <glm/detail/func_trigonometric.inl>
#include <glm/ext/quaternion_geometric.hpp>

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
