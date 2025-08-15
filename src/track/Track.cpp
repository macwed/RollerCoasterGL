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
#include "physics/PTFBuilder.hpp"

#include <iostream>
#include <glm/gtx/quaternion.hpp>

Sample PathSampler::sampleAtS(float s) const
{
    auto [seg, sLocal] = spline_.locateSegmentByS(s);
    if (seg < edge_.size() && edge_[seg].type == EdgeType::Linear)// segment CR (seg) przebiega między węzłami P1, P2 = nodes[seg+1], nodes[seg+2]
    {
        std::size_t i1 = seg + 1, i2 = seg + 2;
        glm::vec3 P1 = spline_.getNode(i1).pos;
        glm::vec3 P2 = spline_.getNode(i2).pos;

        float len = spline_.arcLengthAtSegmentEnd(seg) - spline_.arcLengthAtSegmentStart(seg);
        float u = len > kEps ? std::clamp(sLocal / len, 0.0f, 1.0f) : 0.f;

        glm::vec3 pos = glm::mix(P1, P2, u);
        glm::vec3 dir = P2 - P1;
        glm::vec3 tan = glm::length2(dir) > 1e-12f ? glm::normalize(dir) : glm::vec3(1.f, 0.f, 0.f);

        return {pos, tan};
    }

    return {spline_.getPositionAtS(s), glm::normalize(spline_.getTangentAtS(s))};
}
/*-------------------------------------------------TRACK::TRACK-------------------------------------------------------*/
Track::Track() : vbo_(0), vao_(0), ibo_(0) {}

std::vector<Frame> Track::buildFrames(const Spline& spline, float ds, glm::vec3 globalUp) const
{
    physics::MetaCallbacks meta;
    meta.isInStation = [this](float s)
    {
        return this->isInStation(s);
    };
    meta.stationEdgeFadeWeight = [this](float s)
    {
            return this->stationEdgeFadeWeight(s);
    };
    meta.manualRollAtS = [this, &spline](float s)
    {
        return this->manualRollAtS(spline, s);
    };

    return physics::PTFBuilder::build(spline, ds, globalUp, meta);
}

/*--------------------------------------------TRACK::ApproxSForPoint--------------------------------------------------*/

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

/*----------------------------------------------TRACK::station...-----------------------------------------------------*/

float Track::stationEdgeFadeWeight(float s) const
{
    if (stationIntervals_.empty()) return 0.f;
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
bool Track::isInStation(float s) const {
    for (auto [a,b] : stationIntervals_) {
        if (s >= a && s <= b) return true;
    }
    return false;
}

bool Track::isNearStationEdge(float s) const
{
    if (stationIntervals_.empty()) return false;
    for (auto [a, b] : stationIntervals_)
    {
        if ((s >= a - feather && s < a) || (s > b && s <= b + feather)) return true;
    }
    return false;
}

void Track::buildStationIntervals(const Spline& spline, float sampleStep)
{
    syncMetaWithSpline(spline);

    stationIntervals_.clear();
    const float L = spline.totalLength();

    auto pushInterval = [&](float a, float b)
    {
        if (a <= b) stationIntervals_.emplace_back(a, b);
        else
        {
            stationIntervals_.emplace_back(a, L);
            stationIntervals_.emplace_back(0.f, b);
        }
    };

    for (std::size_t i = 0; i < nodeMeta_.size(); ++i)
    {
        float stationL = nodeMeta_[i].stationLength;
        if (nodeMeta_[i].stationStart && stationL > 0.f)
        {
            const glm::vec3 nodePos = spline.getNode(i).pos;
            float sA = spline.isNodeOnCurve(i) ? spline.sAtNode(i) :
                            approximateSForPoint(spline, nodePos, 0, L, sampleStep);
            float sB = std::min(sA + stationL, L);
            if (sB <= L)
                pushInterval(sA, sB);
            else
                pushInterval(sA, std::fmod(sB, L));
        }
    }

    for (std::size_t i = 0; i + 1 < nodeMeta_.size(); ++i)
    {
        if (nodeMeta_[i].stationStart && nodeMeta_[i + 1].stationEnd)
        {
            const glm::vec3 Apos = spline.getNode(i).pos;
            const glm::vec3 Bpos = spline.getNode(i + 1).pos;
            float sA = spline.isNodeOnCurve(i) ? spline.sAtNode(i) :
                            approximateSForPoint(spline, Apos, 0, spline.totalLength(), sampleStep);
            float sB = spline.isNodeOnCurve(i + 1) ? spline.sAtNode(i + 1) :
                            approximateSForPoint(spline, Bpos, 0, spline.totalLength(), sampleStep);

            if (sB < sA) std::swap(sA, sB);
            pushInterval(sA, sB);
        }
    }
    std::ranges::sort(stationIntervals_);
    std::vector<std::pair<float, float>> merged;
    constexpr float closeEps = 1e-4f;
    for (auto [a, b] : stationIntervals_)
    {
        if (merged.empty() || a > merged.back().second + closeEps) merged.emplace_back(a, b);
        else merged.back().second = std::max(merged.back().second, b);
    }
    stationIntervals_.swap(merged);
}

/*-----------------------------------------------TRACK::roll...-------------------------------------------------------*/

void Track::rebuildRollKeys(const Spline& spline, float sampleStep)
{
    syncMetaWithSpline(spline);

    rollKeys_.clear();
    rollKeys_.reserve(nodeMeta_.size());
    for (std::size_t i = 0; i < nodeMeta_.size(); ++i)
    {
        const glm::vec3 nodePos = spline.getNode(i).pos;
        float s = spline.isNodeOnCurve(i) ? spline.sAtNode(i) :
                    approximateSForPoint(spline, nodePos, 0.f, spline.totalLength(), sampleStep);
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

float Track::manualRollAtS(const Spline& spline, float s) const
{
    if (rollKeys_.empty()) return 0.f;
    if (rollKeys_.size() == 1) return rollKeys_[0].roll;

    float L = spline.totalLength();

    auto wrap01 = [&](float x)
    {
        return std::fmod(std::fmod(x, L) + L, L);
    };

    if (spline.isClosed())
    {
        s = wrap01(s);
    } else
    {
        if (s <= rollKeys_.front().s) return rollKeys_.front().roll;
        if (s >= rollKeys_.back().s) return rollKeys_.back().roll;
    }

    auto it = std::upper_bound(rollKeys_.begin(), rollKeys_.end(), s,
                                                            [](float v, const RollKey& k){ return v < k.s; });
    const RollKey& k1 = (it == rollKeys_.begin()) ? rollKeys_.back() : *(it - 1);
    const RollKey& k2 = (it == rollKeys_.end()) ? rollKeys_.front() : *it;

    float s1 = k1.s;
    float s2 = k2.s;
    float ds = s2 - s1;

    if (spline.isClosed() && ds < 0.f) ds += L;

    if (std::abs(ds) < kEps)
    {
        std::cerr << "[Roll] Duplicate keys at s= " << s1 << std::endl;
        return k1.roll;
    }

    float d = s - s1;
    if (spline.isClosed() && d < 0.f) d += L;
    float t = d / ds; //ds na pewno różne od 0

    return k1.roll * (1.f - t) + k2.roll * t;
}

/*--------------------------------------------------TRACK::meta...----------------------------------------------------*/

void Track::syncMetaWithSpline(const Spline& spline)
{
    const std::size_t n = spline.nodeCount();
    const std::size_t sg = spline.segmentCount();
    nodeMeta_.resize(n);
    edgeMeta_.resize(sg);
}

void Track::rebuildMeta(const Spline& spline) {
    syncMetaWithSpline(spline);
    std::ranges::fill(edgeMeta_, EdgeMeta{EdgeType::CatmullRom});

    buildStationIntervals(spline, 0.05f);

    auto markLinearInStation = [&](const Spline& spl)
    {
        for (std::size_t seg = 0; seg < spl.segmentCount(); ++seg)
        {
            float sA = spl.arcLengthAtSegmentStart(seg);
            float sB = spl.arcLengthAtSegmentEnd(seg);
            float sMid = (sA + sB) / 2.f;

            for (auto [a, b] : stationIntervals_)
            {
                if (sMid >= a && sMid <= b)
                {
                    edgeMeta_[seg].type = EdgeType::Linear;
                    break;
                }
            }
        }
    };
    markLinearInStation(spline);
    rebuildRollKeys(spline, 0.05f);
}

/*-------------------------------------------------TRACK::helpers...--------------------------------------------------*/



/*-------------------------------------------------TRACK::OpenGL&GPU--------------------------------------------------*/

void Track::releaseGL()
{
    if (vao_) {glDeleteVertexArrays(1, &vao_); vao_ = 0;}
    if (vbo_) {glDeleteBuffers(1, &vbo_); vbo_ = 0;}
    if (ibo_) {glDeleteBuffers(1, &ibo_); ibo_ = 0;}
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