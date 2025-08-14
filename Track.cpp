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

std::vector<Frame> Track::buildPTF(const Spline& spline, float ds, glm::vec3 globalUp) const
{
    const float trackLength = spline.totalLength();
    PathSampler sampler(spline, edgeMeta_);

    std::vector<Frame> frames;
    frames.reserve(static_cast<std::size_t>(trackLength / ds) + 2);

    auto s0 = sampler.sampleAtS(0.f);

    glm::vec3 T0 = s0.tan;
    glm::vec3 N0_raw = globalUp -  T0 * glm::dot(globalUp, T0);
    if (glm::length2(N0_raw) < kEpsVertical) //zabezpieczenie gdyby jednak punkt startowy był (prawie) pionowy... pionowa stacja? cmon...
    {
        glm::vec3 tmp = (std::abs(T0.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3 (1, 0, 0));
        N0_raw = tmp - T0 * glm::dot(tmp, T0);
    }
    glm::vec3 N0 = glm::normalize(N0_raw);
    glm::vec3 B0 = glm::normalize(glm::cross(T0, N0));
    N0 = glm::normalize(glm::cross(B0, T0));
    frames.emplace_back(Frame{s0.pos, T0, N0, B0, 0.f});

    //rotacja wektora styczna względem wektora stycznego w punkcie poprzednim i wyliczenie norm,binorm
    for (float s = ds; s < trackLength; s+=ds)
    {
        auto sm = sampler.sampleAtS(s);
        glm::vec3 T_prev = frames.back().T;
        glm::vec3 T_curr = sm.tan;

        glm::vec3 v = glm::cross(T_prev, T_curr);
        float sin_phi = glm::length(v);
        float cos_phi = std::clamp(glm::dot(T_prev, T_curr), -1.0f, 1.0f);

        glm::vec3 N_prev = frames.back().N;
        glm::vec3 N_curr, B_curr;

        if (std::abs(sin_phi) >= kEps)
        {
            float phi = std::atan2(sin_phi, cos_phi);
            glm::vec3 axis = v / sin_phi; //norma
            glm::vec3 N_rot = rotateAroundAxis(N_prev, axis, phi);
            B_curr = glm::normalize(glm::cross(T_curr, N_rot));
            N_curr = glm::normalize(glm::cross(B_curr, T_curr));
        }
        else
        {
            //taki bezpiecznik gdyby jednak vec styczne były równoległe o przeciwnych zwrotach - czasem zjebie ramkę na spojeniach segmentów itp
            if (cos_phi < 0.f) {
                // T_curr ~ -T_prev
                N_curr = -N_prev;
                B_curr = -frames.back().B;
            } else {
                //niemal równoległe o zgodnym kierunku
                N_curr = N_prev;
                B_curr = frames.back().B;
            }
        }

        const bool inStation = isInStation(s);
        if (inStation || isNearStationEdge(s))
        {
            glm::vec3 Ng = globalUp - T_curr * glm::dot(globalUp, T_curr);
            if (glm::length2(Ng) <= kEpsVertical) {
                glm::vec3 fallback = (std::abs(T_curr.y) < 0.9f) ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
                Ng = fallback - T_curr * glm::dot(fallback, T_curr);
            }
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

        float roll = inStation ? 0.f : manualRollAtS(spline, s);
        if (std::abs(roll) > kEps)
        {
            N_curr = rotateAroundAxis(N_curr, T_curr, roll);
            B_curr = glm::normalize(glm::cross(T_curr, N_curr));
            N_curr = glm::normalize(glm::cross(B_curr, T_curr));
        }

        frames.emplace_back(Frame{sm.pos, T_curr, N_curr, B_curr, s});
    }
    auto sl = sampler.sampleAtS(trackLength);
    frames.emplace_back(Frame{
        sl.pos, glm::normalize(sl.tan), frames.back().N, frames.back().B, trackLength});
    return frames;
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
        if (merged.empty() || a > merged.back().second + closeEps) merged.push_back(std::make_pair(a, b));
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

glm::vec3 Track::rotateAroundAxis(const glm::vec3& v, const glm::vec3& axis, float angle)
{
    glm::quat q = glm::angleAxis(angle, axis);
    return q * v;
}

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