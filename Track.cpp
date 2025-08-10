//
// Created by maciej on 02.08.25.
//
#include <cmath>
#include <glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "Track.hpp"

#include <functional>
#include <glm/gtx/quaternion.hpp>

Track::Track() : vbo_(0), vao_(0), ibo_(0) {}
void Track::releaseGL()
{
    if (vao_) {glDeleteVertexArrays(1, &vao_); vao_ = 0;}
    if (vbo_) {glDeleteBuffers(1, &vbo_); vbo_ = 0;}
    if (ibo_) {glDeleteBuffers(1, &ibo_); ibo_ = 0;}
}

void Track::pushPoint(float x, float y, float z)
{
    points_.emplace_back(x, y, z);
}

void Track::popPoint() { points_.pop_back(); }


std::vector<Frame> Track::buildPTF(const Spline& spline, float ds, glm::vec3 globalUp, float l_station,
                                   std::function<float(float)> rollAtS)
{
    const float trackLength = spline.totalLength();

    std::vector<Frame> frames;
    frames.reserve(static_cast<std::size_t>(trackLength / ds) + 2);

    auto Tdir = [&](float s)
    {
        glm::vec3 t = spline.getTangentAtS(s);
        float L2 = glm::length2(t);
        return (L2 > kEps) ? sqrtf(L2) : glm::vec3(0, 0, 1);
    };

    float s = 0.f;

    glm::vec3 T0 = glm::normalize(spline.getTangentAtS(0));
    glm::vec3 N0_raw = globalUp -  T0 * glm::dot(globalUp, T0);
    glm::vec3 N0 = glm::normalize(N0_raw); //zakładam, że odcinek startowy nigdy nie będzie równoległy do globalUp (pionowy)
    glm::vec3 B0 = glm::normalize(glm::cross(T0, N0));
    frames.emplace_back(spline.getPositionAtS(0), T0, N0, B0, 0);

    for (float s = ds; s <= trackLength + 0.5f * ds; s+=ds)
    {
        glm::vec3 T_prev = frames.back().T;
        glm::vec3 T_curr = spline.getTangentAtS(s);

        float sin_phi = glm::length(glm::cross(T_prev, T_curr));

        glm::vec3 N_prev = frames.back().N;
        glm::vec3 N_curr, B_curr;
        if (std::abs(sin_phi) >= kEps)
        {
            float cos_phi = std::clamp(glm::dot(T_prev, T_curr), -1.0f, 1.0f);
            float phi = std::atan2(sin_phi, cos_phi);

            glm::vec3 axis = glm::normalize(glm::cross(T_prev, T_curr));
            glm::vec3 N_rot = rotateAroundAxis(N_prev, axis, phi);
            B_curr = glm::normalize(glm::cross(T_curr, N_rot));
            N_curr = glm::normalize(glm::cross(B_curr, T_curr));
        }
        else
        {
            N_curr = N_prev;
            B_curr = frames.back().B;
        }
        frames.emplace_back(spline.getPositionAtS(s), T_curr, N_curr, B_curr);
    }

    return frames;
}

void Track::uploadToGPU()
{
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (ibo_) glDeleteBuffers(1, &ibo_);

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ibo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(points_.size() * sizeof(glm::vec3)),
        points_.data(),
        GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices_.size() * sizeof(glm::uint32_t)),
        indices_.data(),
        GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void Track::draw() const
{
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES,
        static_cast<GLsizei>(indices_.size()),
        GL_UNSIGNED_INT,
        nullptr);
}