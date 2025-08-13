//
// Created by maciej on 02.08.25.
//
#include <algorithm>
#include <functional>
#include <cmath>
#include <glad.h>
#include <glm/glm.hpp>
#include <limits>
#include "Track.hpp"
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
    const float L = spline.totalLength();
    s0 = std::clamp(s0, 0.0f, L);
    s1 = std::clamp(s1, 0.0f, L);
    if (s1 < s0) std::swap(s0, s1);

    float bestS = s0, bestD2 = std::numeric_limits<float>::infinity();
    for (float s = s0; s <= s1 + 0.5f*ds; s += ds) {
        glm::vec3 q = spline.getPositionAtS(s);
        float d2 = glm::length2(q - p);
        if (d2 < bestD2) { bestD2 = d2; bestS = s; }
    }
    return bestS;
}

void Track::buildStationIntervals(const Spline& spline, float sampleStep)
{
    stationIntervals_.clear();

    for (std::size_t i = 0; i < nodeMeta_.size(); ++i)
    {
        float stationL = nodeMeta_[i].stationLength;
        if (nodeMeta_[i].stationStart && stationL > 0.f)
        {
            const glm::vec3 nodePos = spline.getNode(i).pos;
            float sA = approximateSForPoint(spline, nodePos, 0, spline.totalLength(), sampleStep);
            float sB = std::min(sA + stationL, spline.totalLength());
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

bool Track::isInStation(float s) const {
    for (auto [a,b] : stationIntervals_) {
        if (s >= a && s <= b) return true;
    }
    return false;
}

bool Track::isNearStationEdge(float s) const
{
    for (auto [a, b] : stationIntervals_)
    {
        if ((s >= a - feather && s < a) || (s > b && s <= b + feather)) return true;
    }
    return false;
}

float Track::stationEdgeFadeWeight(float s) const
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

void Track::rebuildRollKeys(const Spline& spline, float sampleStep)
{
    rollKeys_.clear();
    rollKeys_.reserve(nodeMeta_.size());
    for (std::size_t i = 0; i < nodeMeta_.size(); ++i)
    {
        const glm::vec3 nodePos = spline.getNode(i).pos;
        float s = approximateSForPoint(spline, nodePos, 0.f, spline.totalLength(), sampleStep);
        rollKeys_.emplace_back(s, nodeMeta_[i].userRoll);
    }
    std::ranges::sort(rollKeys_, {}, &RollKey::s);

    std::vector<RollKey> merged;
    constexpr float mergeEps = 0.0001f;
    for (const auto& key : rollKeys_)
    {
        if (!merged.empty() && std::abs(key.s - merged.back().s) < mergeEps)
        {
            merged.back().s = (merged.back().s + key.s) / 2;
            merged.back().roll = key.roll;
        } else
        {
            merged.push_back(key);
        }
    }
    rollKeys_.swap(merged);
}

float Track::manualRollAtS(float s) const
{
    if (rollKeys_.empty()) return 0.f;
    if (s <= rollKeys_.front().s) return rollKeys_.front().roll;
    if (s >= rollKeys_.back().s) return rollKeys_.back().roll;

    auto it = std::upper_bound(rollKeys_.begin(), rollKeys_.end(), s, [](float v, const RollKey& k){ return v < k.s; });
    const RollKey& k1 = *(it - 1);
    const RollKey& k2 = *it;

    float t = (s - k1.s) / (k2.s - k1.s);
    return k1.roll * (1.f - t) + k2.roll * t;
}

std::vector<Frame> Track::buildPTF(const Spline& spline, float ds, glm::vec3 globalUp) const
{
    const float trackLength = spline.totalLength();

    std::vector<Frame> frames;
    frames.reserve(static_cast<std::size_t>(trackLength / ds) + 2);

    glm::vec3 T0 = glm::normalize(spline.getTangentAtS(0));
    glm::vec3 N0_raw = globalUp -  T0 * glm::dot(globalUp, T0);
    if (glm::length2(N0_raw) < kEpsVertical) //zabezpieczenie gdyby jednak punkt startowy był (prawie) pionowy... pionowa stacja? cmon...
    {
        glm::vec3 tmp = (std::abs(T0.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3 (1, 0, 0));
        N0_raw = tmp - T0 * glm::dot(tmp, T0);
    }
    glm::vec3 N0 = glm::normalize(N0_raw);
    glm::vec3 B0 = glm::normalize(glm::cross(T0, N0));
    N0 = glm::normalize(glm::cross(B0, T0));
    frames.emplace_back(Frame{spline.getPositionAtS(0), T0, N0, B0, 0.f});

    //rotacja wektora styczna względem wektora stycznego w punkcie poprzednim i wyliczenie norm,binorm
    for (float s = ds; s <= trackLength + 0.5f * ds; s+=ds)
    {
        glm::vec3 T_prev = frames.back().T;
        glm::vec3 T_curr = spline.getTangentAtS(s);

        float sin_phi = glm::length(glm::cross(T_prev, T_curr));
        float cos_phi = std::clamp(glm::dot(T_prev, T_curr), -1.0f, 1.0f);

        glm::vec3 N_prev = frames.back().N;
        glm::vec3 N_curr, B_curr;
        if (std::abs(sin_phi) >= kEps)
        {
            float phi = std::atan2(sin_phi, cos_phi);

            glm::vec3 axis = glm::normalize(glm::cross(T_prev, T_curr));
            glm::vec3 N_rot = rotateAroundAxis(N_prev, axis, phi);
            B_curr = glm::normalize(glm::cross(T_curr, N_rot));
            N_curr = glm::normalize(glm::cross(B_curr, T_curr));
        }
        else
        {
            //taki bezpiecznik gdyby jednak v styczne były równoległe o przeciwnych zwrotach - czasem zjebie ramkę na spojeniach segmentów itp
            if (cos_phi < 0.f)
            {
                //obrót o pi wokół poprzedniej dobrej osi
                float ref = glm::length2(glm::cross(T_prev, globalUp));
                glm::vec3 axis = ref > kEpsVertical ? glm::vec3(0, 1, 0) : glm::vec3 (1, 0, 0);
                axis = glm::normalize(axis);
                glm::vec3 N_rot = rotateAroundAxis(N_prev, axis, glm::pi<float>());
                B_curr = glm::normalize(glm::cross(T_curr, N_rot));
                N_curr = glm::normalize(glm::cross(B_curr, T_curr));
            } else
            {
                //niemal równoległe, ale w zgodnym kierunku
                N_curr = N_prev;
                B_curr = frames.back().B;
            }
        }

        const bool inStation = isInStation(s);
        if (inStation || isNearStationEdge(s)) {
            glm::vec3 Ng = globalUp - T_curr * glm::dot(globalUp, T_curr);
            if (glm::length2(Ng) > kEpsVertical) {
                Ng = glm::normalize(Ng);
                glm::vec3 Bg = glm::normalize(glm::cross(T_curr, Ng));
                Ng = glm::normalize(glm::cross(Bg, T_curr));
                if (inStation) {
                    N_curr = Ng;
                    B_curr = Bg;
                } else {
                    float w = stationEdgeFadeWeight(s);
                    N_curr = glm::normalize(glm::mix(N_curr, Ng, w));
                    B_curr = glm::normalize(glm::cross(T_curr, N_curr));
                    N_curr = glm::normalize(glm::cross(B_curr, T_curr));
                }
            }
        }

        float roll = inStation ? 0.f : manualRollAtS(s);
        if (std::abs(roll) > kEps)
        {
            N_curr = rotateAroundAxis(N_curr, T_curr, roll);
            B_curr = glm::normalize(glm::cross(T_curr, N_curr));
            N_curr = glm::normalize(glm::cross(B_curr, T_curr));
        }

        frames.emplace_back(Frame{spline.getPositionAtS(s), T_curr, N_curr, B_curr, s});
    }

    return frames;
}

glm::vec3 Track::rotateAroundAxis(const glm::vec3& v, const glm::vec3& axis, float angle)
{
    glm::quat q = glm::angleAxis(angle, axis);
    return q * v;
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