//
// Created by maciej on 18.08.25.
//

#include "TrackComponent.hpp"

#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
#include <limits>

#include "common/TrackTypes.hpp"
#include "math/Spline.hpp"
#include "physics/PTF.hpp"
#include "physics/PathSampler.hpp"

namespace rc::gameplay {

    constexpr float eps = 1e-4f;
    constexpr float mergeEps = 1e-4f;

    static inline float clamp01(float x) {
        return std::max(0.0f, std::min(1.0f, x));
    }

    float TrackComponent::approximateSForPoint_(const math::Spline& spline, const glm::vec3& p, float s0, float s1,
                                                float ds) {
        const float L = spline.totalLength();
        s0 = std::clamp(s0, 0.0f, L);
        s1 = std::clamp(s1, 0.0f, L);
        if (s1 < s0)
            std::swap(s0, s1);

        float bestS = s0, bestD2 = std::numeric_limits<float>::infinity();
        for (float s = s0; s <= s1 + 0.5f * ds; s += ds) {
            glm::vec3 q = spline.getPositionAtS(s);
            float d2 = glm::dot(q - p, q - p);
            if (d2 < bestD2) {
                bestD2 = d2;
                bestS = s;
            }
        }
        return bestS;
    }

    void TrackComponent::rebuildLUT_() {
        spline_.rebuildArcLengthLUT(64);
    }

    void TrackComponent::syncMetaWithSpline_() {
        nodeMeta_.resize(spline_.nodeCount());
        edgeMeta_.resize(spline_.segmentCount());
    }

    void TrackComponent::buildStationIntervals_() {
        stations_.clear();
        if (spline_.segmentCount() == 0)
            return;

        const float L = spline_.totalLength();
        auto pushInterval = [&](float a, float b) {
            if (a <= b)
                stations_.emplace_back(a, b);
            else {
                stations_.emplace_back(a, L);
                stations_.emplace_back(0.f, b);
            }
        };

        // budowa stacji przez 1 węzeł + długość
        for (std::size_t i = 0; i < nodeMeta_.size(); ++i) {
            const common::NodeMeta& m = nodeMeta_[i];
            const float stationL = m.length;
            if (m.stationStart && stationL > 0.f) {
                const glm::vec3 pos = spline_.getNode(i).pos;
                float sA = spline_.isNodeOnCurve(i) ? spline_.sAtNode(i)
                                                    : approximateSForPoint_(spline_, pos, 0.f, L, 0.05f);
                float sB = sA + stationL;
                if (sB <= L)
                    pushInterval(sA, sB);
                else
                    pushInterval(sA, std::fmod(sB, L));
            }
        }

        // przez 2 węzły start/end
        for (std::size_t i = 0; i + 1 < edgeMeta_.size(); ++i) {
            if (nodeMeta_[i].stationStart && nodeMeta_[i + 1].stationEnd) {
                float sA = spline_.isNodeOnCurve(i)
                                   ? spline_.sAtNode(i)
                                   : approximateSForPoint_(spline_, spline_.getNode(i).pos, 0.f, L, 0.05f);
                float sB = spline_.isNodeOnCurve(i + 1)
                                   ? spline_.sAtNode(i + 1)
                                   : approximateSForPoint_(spline_, spline_.getNode(i + 1).pos, 0.f, L, 0.05f);

                if (sB < sA)
                    std::swap(sA, sB);
                pushInterval(sA, sB);
            }
        }

        // sort i merge
        std::ranges::sort(stations_);
        std::vector<std::pair<float, float>> merged;
        for (auto [a, b]: stations_) {
            if (merged.empty() || a > merged.back().second + eps)
                merged.emplace_back(a, b);
            else
                merged.back().second = std::max(merged.back().second, b);
        }
        stations_.swap(merged);
    }

    void TrackComponent::rebuildRollKeys_() {
        rollKeys_.clear();
        rollKeys_.reserve(nodeMeta_.size());
        const float L = spline_.totalLength();

        for (std::size_t i = 0; i < nodeMeta_.size(); ++i) {
            float s = spline_.isNodeOnCurve(i) ? spline_.sAtNode(i)
                                               : approximateSForPoint_(spline_, spline_.getNode(i).pos, 0.f, L, 0.05f);
            rollKeys_.push_back({s, nodeMeta_[i].userRoll});
        }
        std::ranges::sort(rollKeys_, [](const common::RollKey& a, const common::RollKey& b) { return a.s < b.s; });

        std::vector<common::RollKey> merged;
        for (const auto& k: rollKeys_) {
            if (!merged.empty() && std::abs(k.s - merged.back().s) < mergeEps)
                merged.back().roll = k.roll;
            else
                merged.push_back(k);
        }
        rollKeys_.swap(merged);
    }
    void TrackComponent::buildFrames_() {
        physics::PathSampler sampler(spline_, edgeMeta_);

        physics::MetaCallbacks cb;
        cb.isInStation = [this](float s) { return this->isInStation(s); };
        cb.stationEdgeFadeWeight = [this](float s) { return this->stationEdgeFadeWeight(s); };
        cb.manualRollAtS = [this](float s) { return this->manualRollAtS(s); };

        frames_ = buildFrames(sampler, ds_, up_, cb);
    }

    //---------------------------------public API-----------------------------------------------

    void TrackComponent::rebuild() {
        if (dirtySpline_) {
            rebuildLUT_();
            syncMetaWithSpline_();
            dirtyMeta_ = true;
            dirtySpline_ = false;
        }
        if (dirtyMeta_) {
            buildStationIntervals_();
            rebuildRollKeys_();
            dirtyFrames_ = true;
            dirtyMeta_ = false;
        }
        if (dirtyFrames_) {
            buildFrames_();
            dirtyFrames_ = false;
        }
    }

    bool TrackComponent::isInStation(float s) const {
        for (auto [a, b]: stations_) {
            if (s >= a && s <= b)
                return true;
        }
        return false;
    }

    float TrackComponent::stationEdgeFadeWeight(float s) const {
        if (stations_.empty())
            return 0.f;
        for (auto [a, b]: stations_) {
            if (s >= a - feather_ && s < a) {
                float t = (s - (a - feather_)) / feather_;
                t = clamp01(t);
                return t * t * (3.f - 2.f * t);
            }
            if (s > b && s <= b + feather_) {
                float t = 1.f - (s - b) / feather_;
                t = clamp01(t);
                return t * t * (3.f - 2.f * t);
            }
        }
        return 0.f;
    }

    float TrackComponent::manualRollAtS(float s) const {
        if (rollKeys_.empty())
            return 0.f;
        if (rollKeys_.size() == 1)
            return rollKeys_[0].roll;

        const float L = spline_.totalLength();
        auto wrap = [&](float x) { return std::fmod(std::fmod(x, L) + L, L); };

        if (spline_.isClosed())
            s = wrap(s);
        else {
            if (s <= rollKeys_.front().s)
                return rollKeys_.front().roll;
            if (s >= rollKeys_.back().s)
                return rollKeys_.back().roll;
        }

        auto it = std::upper_bound(rollKeys_.begin(), rollKeys_.end(), s,
                                   [](float v, const common::RollKey& k) { return v < k.s; });
        const common::RollKey& k1 = (it == rollKeys_.begin()) ? rollKeys_.back() : *(it - 1);
        const common::RollKey& k2 = (it == rollKeys_.end()) ? rollKeys_.front() : *it;

        float s1 = k1.s, s2 = k2.s;
        float ds = s2 - s1;
        if (spline_.isClosed() && ds < 0.f)
            ds += L;
        if (std::abs(ds) < 1e-6f)
            return k1.roll;

        float d = s - s1;
        if (spline_.isClosed() && d < 0.f)
            d += L;
        float t = d / ds;
        return k1.roll * (1.f - t) + k2.roll * t;
    }

    glm::vec3 TrackComponent::positionAtS(float s) const {
        return spline_.getPositionAtS(s);
    }

    glm::vec3 TrackComponent::tangentAtS(float s) const {
        glm::vec3 tan = spline_.getTangentAtS(s);
        const float len2 = (glm::dot(tan, tan));
        tan = (len2 > physics::kEps2) ? tan * glm::inversesqrt(len2) : glm::vec3(1.f, 0.f, 0.f);
        return tan;
    }

    common::Frame TrackComponent::frameAtS(float s) const {
        if (frames_.empty())
            return {positionAtS(s), tangentAtS(s), {0.f, 1.f, 0.f}, {1.f, 0.f, 0.f}, s};

        if (s <= frames_.front().s)
            return frames_.front();
        if (s >= frames_.back().s)
            return frames_.back();

        auto it = std::upper_bound(frames_.begin(), frames_.end(), s,
                                   [](float v, const common::Frame& f) { return v < f.s; });

        const common::Frame& a = *(it - 1);
        const common::Frame& b = *it;
        float t = (s - a.s) / (b.s - a.s);
        t = clamp01(t);

        common::Frame f;
        f.s = s;
        f.pos = glm::mix(a.pos, b.pos, t);
        f.T = glm::normalize(glm::mix(a.T, b.T, t));
        f.N = glm::normalize(glm::mix(a.N, b.N, t));
        f.B = glm::normalize(glm::cross(f.T, f.N));
        f.N = glm::normalize(glm::cross(f.B, f.T));
        return f;
    }
    void TrackComponent::setClosed(bool v) {
        spline_.setClosed(v);
        if (v && spline_.nodeCount() >= 4) {
            auto P0 = spline_.getNode(0).pos;
            auto Pn = spline_.getNode(spline_.nodeCount() - 1).pos;
            float d = glm::length(Pn - P0);
            constexpr float snap = 0.25f;
            const float stitch = 4.0f;
            if (d < snap) {
                spline_.moveNode(spline_.nodeCount() - 1, P0);
            } else if (d > stitch) {
                glm::vec3 Q1 = glm::mix(Pn, P0, 0.33f);
                glm::vec3 Q2 = glm::mix(Pn, P0, 0.66f);
                spline_.insertNode(spline_.nodeCount(), {Q1});
                spline_.insertNode(spline_.nodeCount(), {Q2});
            }
        }
        markDirty();
    }
} // namespace rc::gameplay
