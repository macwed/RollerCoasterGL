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
            rollKeys_.push_back({s, spline_.getNode(i).roll});
        }
        std::ranges::sort(rollKeys_, [](const common::RollKey& a, const common::RollKey& b) { return a.s < b.s; });

        // merge by s and unwrap angles to keep continuity (avoid large jumps across +/-pi)
        std::vector<common::RollKey> merged;
        auto wrapPi = [](float a) {
            // wrap to (-pi, pi]
            float x = std::fmod(a + glm::pi<float>(), glm::two_pi<float>());
            if (x <= 0.f) x += glm::two_pi<float>();
            return x - glm::pi<float>();
        };

        float prevS = -std::numeric_limits<float>::infinity();
        float prevAdj = 0.f;
        bool first = true;
        for (const auto& k : rollKeys_) {
            if (!merged.empty() && std::abs(k.s - merged.back().s) < mergeEps) {
                // overwrite same-s key
                merged.back().roll = k.roll;
                continue;
            }
            float r = k.roll;
            if (first) {
                prevAdj = r;
                merged.push_back({k.s, prevAdj});
                first = false;
            } else {
                float delta = wrapPi(r - prevAdj);
                prevAdj = prevAdj + delta;
                merged.push_back({k.s, prevAdj});
            }
            prevS = k.s;
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
        if (rollKeys_.empty()) return 0.f;
        if (rollKeys_.size() == 1) return rollKeys_.front().roll;

        const float L = spline_.totalLength();
        auto wrapS = [&](float x) { return std::fmod(std::fmod(x, L) + L, L); };
        auto wrapPi = [](float a) {
            float x = std::fmod(a + glm::pi<float>(), glm::two_pi<float>());
            if (x <= 0.f) x += glm::two_pi<float>();
            return x - glm::pi<float>();
        };

        float ss = s;
        if (spline_.isClosed()) ss = wrapS(s);
        else {
            if (ss <= rollKeys_.front().s) return rollKeys_.front().roll;
            if (ss >= rollKeys_.back().s)  return rollKeys_.back().roll;
        }

        auto it = std::upper_bound(rollKeys_.begin(), rollKeys_.end(), ss,
                                   [](float v, const common::RollKey& k) { return v < k.s; });
        const common::RollKey& k1 = (it == rollKeys_.begin()) ? rollKeys_.back() : *(it - 1);
        const common::RollKey& k2 = (it == rollKeys_.end()) ? rollKeys_.front() : *it;

        float s1 = k1.s, s2 = k2.s;
        float ds = s2 - s1;
        if (spline_.isClosed() && ds < 0.f) ds += L;
        if (std::abs(ds) < 1e-6f) return k1.roll;

        float d = ss - s1;
        if (spline_.isClosed() && d < 0.f) d += L;
        float t = d / ds;

        float r1 = k1.roll;
        float r2 = k2.roll;
        float delta = wrapPi(r2 - r1);
        return r1 + delta * t;
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

    void TrackComponent::setNodeRoll(std::size_t i, float roll) {
        spline_.setNodeRoll(i, roll);
        dirtyMeta_ = true;
        dirtyFrames_ = true;
    }

    // -------------------- Edge helpers --------------------
    static inline bool nodeToSeg(const math::Spline& s, std::size_t nodeIdx, std::size_t& segOut) {
        const std::size_t segCount = s.segmentCount();
        if (segCount == 0) return false;
        if (s.isClosed()) {
            segOut = nodeIdx % s.nodeCount();
            return true;
        }
        if (!s.isNodeOnCurve(nodeIdx)) return false;
        segOut = s.segmentIndexStartingAtNode(nodeIdx);
        return segOut < segCount;
    }

    bool TrackComponent::setLinearBySegment(std::size_t segIdx) {
        if (segIdx >= edgeMeta_.size()) return false;
        auto& e = edgeMeta_[segIdx];
        e.type = common::EdgeType::Linear;
        dirtyFrames_ = true;
        return true;
    }
    bool TrackComponent::setLinearByNode(std::size_t nodeIdx) {
        std::size_t seg{}; if (!nodeToSeg(spline_, nodeIdx, seg)) return false;
        return setLinearBySegment(seg);
    }

    bool TrackComponent::setCircularBySegment(std::size_t segIdx, glm::vec3 center, glm::vec3 normal,
                                              float radius, float turns, bool shortest) {
        if (segIdx >= edgeMeta_.size()) return false;
        auto& e = edgeMeta_[segIdx];
        e.type = common::EdgeType::Circular;
        e.circleCenter = center;
        e.circleNormal = normal;
        e.circleRadius = radius;
        e.circleTurns = turns;
        e.circleShortest = shortest;
        dirtyFrames_ = true;
        return true;
    }
    bool TrackComponent::setCircularByNode(std::size_t nodeIdx, glm::vec3 center, glm::vec3 normal,
                                           float radius, float turns, bool shortest) {
        std::size_t seg{}; if (!nodeToSeg(spline_, nodeIdx, seg)) return false;
        return setCircularBySegment(seg, center, normal, radius, turns, shortest);
    }

    //DO NAPRAWY
    bool TrackComponent::setHelixBySegment(std::size_t segIdx, glm::vec3 axisPoint, glm::vec3 axisDir,
                                           float radius, float pitch, float turns) {
        if (segIdx >= edgeMeta_.size()) return false;
        auto& e = edgeMeta_[segIdx];
        e.type = common::EdgeType::Helix;
        e.helixAxisPoint = axisPoint;
        e.helixAxisDir = axisDir;
        e.helixRadius = radius;
        e.helixPitch = pitch;
        e.helixTurns = turns;
        dirtyFrames_ = true;
        return true;
    }
    bool TrackComponent::setHelixByNode(std::size_t nodeIdx, glm::vec3 axisPoint, glm::vec3 axisDir,
                                        float radius, float pitch, float turns) {
        std::size_t seg{}; if (!nodeToSeg(spline_, nodeIdx, seg)) return false;
        return setHelixBySegment(seg, axisPoint, axisDir, radius, pitch, turns);
    }
} // namespace rc::gameplay
