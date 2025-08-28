//
// Created by maciej on 28.07.25.
//

#ifndef FREEFLYCAM_HPP
#define FREEFLYCAM_HPP
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>


class FreeFlyCam {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;

    float yaw;
    float pitch;
    float moveSpeed;
    float mouseSensitivity;

    FreeFlyCam(glm::vec3 startPos);

    void lookAtTarget(const glm::vec3& target);

    void processKeyboard(const bool* keys, float deltaTime);
    void processMouse(float xOffset, float yOffset);
    glm::mat4 getViewMatrix() const;

private:
    void updateVectors();
};


#endif // FREEFLYCAM_HPP
