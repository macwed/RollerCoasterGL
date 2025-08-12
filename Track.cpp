//
// Created by maciej on 02.08.25.
//
#include <cmath>
#include <glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "Track.hpp"

#include <algorithm>
#include <functional>
#include <glm/gtx/quaternion.hpp>

Track::Track() : vbo_(0), vao_(0), ibo_(0) {}
void Track::releaseGL()
{
    if (vao_) {glDeleteVertexArrays(1, &vao_); vao_ = 0;}
    if (vbo_) {glDeleteBuffers(1, &vbo_); vbo_ = 0;}
    if (ibo_) {glDeleteBuffers(1, &ibo_); ibo_ = 0;}
}

float Track::approximateSForPoint(const Spline& spline, const glm::vec3& p, float s0, float s1, float ds)
{
    float bestS = s0, bestD2 = std::numeric_limits<float>::infinity();
    for (float s = s0; s <= s1; s += ds)
    {
        glm::vec3 q = spline.getPositionAtS(s);
        float d2 = glm::length2(q - p);
        if (d2 < bestD2)
        {
            bestD2 = d2;
            bestS = s;
        }
    }
    return bestS;
}

void Track::buildStationIntervals(const Spline& spline, float sampleStep)
{
    stationIntervals_.clear();

    for (std::size_t i = 0; i < nodeMeta_.size(); ++i)
    {
        if (nodeMeta_[i].stationStart && nodeMeta_[i].stationLength > 0.f)
        {
            const glm::vec3 nodePos = spline.getNode(i).pos;
            float sA = approximateSForPoint(spline, nodePos, 0, spline.totalLength(), sampleStep);
            float sB = std::min(sA + nodeMeta_[i].stationLength, spline.totalLength());
            stationIntervals_.emplace_back(sA, sB);
        }
    }

    for (std::size_t i = 0; i + 1 < nodeMeta_.size(); ++i)
    {
        if (nodeMeta_[i].stationStart && nodeMeta_[i + 1].stationEnd)
        {
            const glm::vec3 Apos = spline.getNode(i).pos;
            const glm::vec3 Bpos = spline.getNode(i + 1).pos;
            float sA = approximateSForPoint(spline, Apos, 0, spline.totalLength(), sampleStep);
            float sB = approximateSForPoint(spline, Bpos, 0, spline.totalLength(), sampleStep);

            if (sB < sA) std::swap(sA, sB);
            stationIntervals_.emplace_back(sA, sB);
        }
    }
    std::ranges::sort(stationIntervals_);
    std::vector<std::pair<float, float>> merged;
    for (auto [a, b] : stationIntervals_)
    {
        if (merged.empty() || a > merged.back().second) merged.push_back(std::make_pair(a, b));
        else merged.back().second = std::max(merged.back().second, b);
    }
    stationIntervals_.swap(merged);
}

bool Track::isNearStationEdge(float s, float feather) const
{
    for (auto [a, b] : stationIntervals_)
    {
        if ((s >= a - feather && s < a) || s > b && s <= b + feather) return true;
    }
    return false;
}

float Track::stationEdgeFadeWeight(float s, float feather) const
{
    for (auto [a, b] : stationIntervals_)
    {
        if (s >= a - feather && s < a)
        {
            float t = (s - (a - feather)) / feather;
            return t * t * (3.f - 2.f * t);
        }
        if (s > b && s <= b + feather)
        {
            float t = 1.f - (s - b) / feather;
            return t * t * (3.f - 2.f * t);
        }
    }
    return 0.f;
}

std::vector<Frame> Track::buildPTF(const Spline& spline, float ds, glm::vec3 globalUp, float l_station,
                                   std::function<float(float)> rollAtS)
{
    const float trackLength = spline.totalLength();

    std::vector<Frame> frames;
    frames.reserve(static_cast<std::size_t>(trackLength / ds) + 2);

    glm::vec3 T0 = glm::normalize(spline.getTangentAtS(0));
    glm::vec3 N0_raw = globalUp -  T0 * glm::dot(globalUp, T0);
    if (glm::length(N0_raw) < 1e-8f) //zabezpieczenie gdyby jednak punkt startowy był (prawie) pionowy... pionowa stacja? cmon...
    {
        glm::vec3 tmp = (std::abs(T0.y < 0.9f) ? glm::vec3(0, 1, 0) : glm::vec3 (1, 0, 0));
        N0_raw = tmp - T0 * glm::dot(tmp, T0);
    }
    glm::vec3 N0 = glm::normalize(N0_raw);
    glm::vec3 B0 = glm::normalize(glm::cross(T0, N0));
    N0 = glm::normalize(glm::cross(B0, T0));
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