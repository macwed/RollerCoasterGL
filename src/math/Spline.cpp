//
// Created by maciej on 07.08.25.
//
#include "Spline.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <ranges>
#include <stdexcept>
#include <vector>

namespace rc::math {
    void Spline::addNode(const Node& node) {
        nodes_.push_back(node);
    }
    void Spline::insertNode(std::size_t i, const Node& node) {
        if (i > nodes_.size())
            throw std::out_of_range("Spline::insertNode index out of range");
        nodes_.insert(nodes_.begin() + static_cast<ptrdiff_t>(i), node);
    }
    void Spline::moveNode(std::size_t i, const glm::vec3& newPos) {
        if (i >= nodes_.size())
            throw std::out_of_range("Spline::moveNode index out of range");
        nodes_[i].pos = newPos;
    }
    void Spline::removeNode(std::size_t i) {
        if (i >= nodes_.size())
            throw std::out_of_range("Spline::removeNode index out of range");
        nodes_.erase(nodes_.begin() + static_cast<ptrdiff_t>(i));
    }

    bool Spline::isNodeOnCurve(std::size_t i) const {
        if (closed_)
            return (nodes_.size() >= 4);
        return (i > 0 && i + 1 < nodes_.size());
    }

    float Spline::sAtNode(std::size_t i) const {
        if (closed_) {
            if (segmentCount() == 0)
                return 0.f;
            return segPrefix_[i % segPrefix_.size()]; // początek segmentu i
        }
        if (!isNodeOnCurve(i))
            throw std::out_of_range("node not on curve");
        if (i == 1)
            return 0.f;
        std::size_t segEnd = i - 1;
        return segPrefix_[segEnd] + lut_[segEnd].length;
    }

    std::size_t Spline::segmentIndexEndingAtNode(std::size_t i) const {
        if (closed_)
            return static_cast<std::size_t>((i + nodes_.size() - 1) % nodes_.size());
        if (!isNodeOnCurve(i))
            throw std::out_of_range("node not on curve");
        return i - 1;
    }

    std::size_t Spline::segmentIndexStartingAtNode(std::size_t i) const {
        if (closed_)
            return static_cast<std::size_t>(i % nodes_.size());
        if (!isNodeOnCurve(i))
            throw std::out_of_range("node not on curve");
        return i;
    }


    std::size_t Spline::segmentCount() const {
        if (closed_)
            return (nodes_.size() >= 4) ? nodes_.size() : 0;
        return nodes_.size() >= 4 ? nodes_.size() - 3 : 0;
    }

    glm::vec3 Spline::getPosition(std::size_t segmentIndex, float t) const {
        const std::size_t n = nodes_.size();
        if (segmentCount() == 0)
            throw std::out_of_range("Spline::getPosition no segments");
        if (t < 0.f)
            t = 0.f;
        else if (t > 1.f)
            t = 1.f;

        constexpr float alpha = 0.5f; // centripetal

        glm::vec3 P0, P1, P2, P3;
        if (closed_) {
            auto w = [n](std::ptrdiff_t k) {
                auto n1 = static_cast<std::ptrdiff_t>(n);
                return ((k % n1 + n1) % n1);
            };
            std::ptrdiff_t i = static_cast<std::ptrdiff_t>(segmentIndex);
            P0 = nodes_[w(i - 1)].pos;
            P1 = nodes_[w(i + 0)].pos;
            P2 = nodes_[w(i + 1)].pos;
            P3 = nodes_[w(i + 2)].pos;
        } else {
            const auto i = static_cast<std::ptrdiff_t>(segmentIndex);
            const auto im1 = i - 1;
            const auto ip2 = i + 2;
            P0 = (im1 >= 0) ? nodes_[static_cast<std::size_t>(im1)].pos : (2.f * nodes_[0].pos - nodes_[1].pos);
            P1 = nodes_[static_cast<std::size_t>(i)].pos;
            P2 = nodes_[static_cast<std::size_t>(i + 1)].pos;
            P3 = (ip2 < static_cast<std::ptrdiff_t>(n)) ? nodes_[static_cast<std::size_t>(ip2)].pos
                                                        : (2.f * nodes_[n - 1].pos - nodes_[n - 2].pos);
        }

        // centripetal param:
        const float t0 = 0.0f;
        const float t1 = t0 + std::pow(glm::length(P1 - P0), alpha);
        const float t2 = t1 + std::pow(glm::length(P2 - P1), alpha);
        const float t3 = t2 + std::pow(glm::length(P3 - P2), alpha);
        const float dt = std::max(t2 - t1, 1e-6f);
        const float tt = t1 + t * dt; // rzeczywisty parametr w [t1,t2]

        glm::vec3 m1 = ((P1 - P0) / std::max(t1 - t0, 1e-6f) - (P2 - P0) / std::max(t2 - t0, 1e-6f) +
                        (P2 - P1) / std::max(t2 - t1, 1e-6f)) *
                       dt;

        glm::vec3 m2 = ((P2 - P1) / std::max(t2 - t1, 1e-6f) - (P3 - P1) / std::max(t3 - t1, 1e-6f) +
                        (P3 - P2) / std::max(t3 - t2, 1e-6f)) *
                       dt;

        const float u = (tt - t1) / dt;
        const float u2 = u * u;
        const float u3 = u2 * u;
        const float h00 = 2 * u3 - 3 * u2 + 1;
        const float h10 = u3 - 2 * u2 + u;
        const float h01 = -2 * u3 + 3 * u2;
        const float h11 = u3 - u2;

        return h00 * P1 + h10 * m1 + h01 * P2 + h11 * m2;
    }

    glm::vec3 Spline::getDerivative(std::size_t segmentIndex, float t) const {
        const std::size_t n = nodes_.size();
        if (segmentCount() == 0)
            throw std::out_of_range("Spline::getDerivative no segments");
        if (t < 0.f)
            t = 0.f;
        else if (t > 1.f)
            t = 1.f;

        constexpr float alpha = 0.5f;

        glm::vec3 P0, P1, P2, P3;
        if (closed_) {
            auto w = [n](std::ptrdiff_t k) {
                auto n1 = static_cast<std::ptrdiff_t>(n);
                return ((k % n1 + n1) % n1);
            };
            std::ptrdiff_t i = static_cast<std::ptrdiff_t>(segmentIndex);
            P0 = nodes_[w(i - 1)].pos;
            P1 = nodes_[w(i + 0)].pos;
            P2 = nodes_[w(i + 1)].pos;
            P3 = nodes_[w(i + 2)].pos;
        } else {
            const auto i = static_cast<std::ptrdiff_t>(segmentIndex);
            const int im1 = i - 1;
            const int ip2 = i + 2;
            P0 = (im1 >= 0) ? nodes_[static_cast<std::size_t>(im1)].pos : (2.f * nodes_[0].pos - nodes_[1].pos);
            P1 = nodes_[static_cast<std::size_t>(i)].pos;
            P2 = nodes_[static_cast<std::size_t>(i + 1)].pos;
            P3 = (ip2 < static_cast<int>(n)) ? nodes_[static_cast<std::size_t>(ip2)].pos
                                             : (2.f * nodes_[n - 1].pos - nodes_[n - 2].pos);
        }

        const float t0 = 0.0f;
        const float t1 = t0 + std::pow(glm::length(P1 - P0), alpha);
        const float t2 = t1 + std::pow(glm::length(P2 - P1), alpha);
        const float t3 = t2 + std::pow(glm::length(P3 - P2), alpha);
        const float dt = std::max(t2 - t1, 1e-6f);
        const float tt = t1 + t * dt;

        glm::vec3 m1 = ((P1 - P0) / std::max(t1 - t0, 1e-6f) - (P2 - P0) / std::max(t2 - t0, 1e-6f) +
                        (P2 - P1) / std::max(t2 - t1, 1e-6f)) *
                       dt;

        glm::vec3 m2 = ((P2 - P1) / std::max(t2 - t1, 1e-6f) - (P3 - P1) / std::max(t3 - t1, 1e-6f) +
                        (P3 - P2) / std::max(t3 - t2, 1e-6f)) *
                       dt;

        const float u = (tt - t1) / dt;
        const float u2 = u * u;

        // Pochodna Hermite’a po u
        const float dh00 = 6 * u2 - 6 * u;
        const float dh10 = 3 * u2 - 4 * u + 1;
        const float dh01 = -6 * u2 + 6 * u;
        const float dh11 = 3 * u2 - 2 * u;

        // dC/du
        glm::vec3 dCdu = dh00 * P1 + dh10 * m1 + dh01 * P2 + dh11 * m2;
        // dC/dt = (dC/du) * du/dt, gdzie u = (t - t1)/(t2 - t1) -> du/dt = 1/dt
        return dCdu / dt;
    }

    glm::vec3 Spline::getTangent(std::size_t segmentIndex, float t) const {
        if (segmentCount() == 0) {
            throw std::out_of_range("Spline::getTangent invalid segment index");
        }
        assert(segmentIndex < segmentCount());

        t = std::clamp(t, 0.0f, 1.0f);

        // pochodna po t
        glm::vec3 derivative = getDerivative(segmentIndex, t);
        float len = glm::length(derivative);
        if (len >= kEps)
            return derivative / len;
        const auto n = nodes_.size();

        const glm::vec3* P1 = &nodes_[segmentIndex + 1].pos;
        const glm::vec3* P2 = &nodes_[segmentIndex + 2].pos;
        if (closed_) {
            P1 = &nodes_[wrap(segmentIndex + 0, n)].pos;
            P2 = &nodes_[wrap(segmentIndex + 1, n)].pos;
        }
        glm::vec3 seg = *P2 - *P1;
        float segLen = glm::length(seg);
        if (segLen >= kEps) {
            return seg / segLen;
        }

        const glm::vec3* P0 = &nodes_[segmentIndex + 0].pos;
        const glm::vec3* P3 = &nodes_[segmentIndex + 3].pos;
        if (closed_) {
            P0 = &nodes_[wrap(segmentIndex - 1, n)].pos;
            P3 = &nodes_[wrap(segmentIndex + 2, n)].pos;
        }
        glm::vec3 wide = *P3 - *P0;
        float wideLen = glm::length(wide);
        if (wideLen >= kEps) {
            return wide / wideLen;
        }
        return {1.0f, 0.0f, 0.0f};
    }

    void Spline::rebuildArcLengthLUT(std::size_t minSamplesPerSegment) {
        const auto segCount = segmentCount();
        if (segCount == 0) {
            lut_.clear();
            segPrefix_.clear();
            totalLength_ = 0.f;
            return;
        }

        if (minSamplesPerSegment < 2)
            minSamplesPerSegment = 2;
        lut_.clear();
        lut_.reserve(segCount);
        segPrefix_.assign(segCount, 0.f);
        for (std::size_t seg = 0; seg < segCount; ++seg) {
            SegmentLUT segLUT;
            segLUT.samples.resize(minSamplesPerSegment + 1);
            glm::vec3 prevPos = getPosition(seg, 0);
            float s = 0.f;
            segLUT.samples[0] = {0.f, 0.f, prevPos};
            for (std::size_t i = 1; i <= minSamplesPerSegment; ++i) {
                float u = static_cast<float>(i) / static_cast<float>(minSamplesPerSegment);
                glm::vec3 pos = getPosition(seg, u);
                float ds = glm::length(pos - prevPos);
                if (ds < kEps)
                    ds = 0.f;
                s += ds;
                segLUT.samples[i] = {u, s, pos};
                prevPos = pos;
            }
            segLUT.length = s;
            lut_.push_back(std::move(segLUT));
        }

        for (std::size_t i = 1; i < segPrefix_.size(); ++i) {
            segPrefix_[i] = segPrefix_[i - 1] + lut_[i - 1].length;
        }
        totalLength_ = segPrefix_.back() + lut_.back().length;
    }

    float Spline::arcLengthAtSegmentStart(std::size_t seg) const {
        return segPrefix_[seg];
    }

    float Spline::arcLengthAtSegmentEnd(std::size_t seg) const {
        return segPrefix_[seg] + lut_[seg].length;
    }


    bool Spline::isClosed() const {
        return closed_;
    }

    std::pair<std::size_t, float> Spline::locateSegmentByS(float s) const {
        const float L = totalLength();
        if (segmentCount() == 0)
            throw std::out_of_range("Spline::locateSegmentByS no segments");
        if (L <= 0)
            return std::make_pair(0, 0.f);
        if (isClosed()) {
            s = std::fmod(std::fmod(s, L) + L, L);
        } else {
            s = std::clamp(s, 0.f, L);
        }
        auto it = std::ranges::upper_bound(segPrefix_, s);
        if (it == segPrefix_.begin()) // s == 0
        {
            return std::make_pair(0, s);
        }
        if (it == segPrefix_.end()) // s == totalLength_
        {
            std::size_t last = segPrefix_.size() - 1;
            return std::make_pair(last, s - segPrefix_[last]); // gdy sLocal == length ostatniego segmentu
        }
        auto k = static_cast<std::size_t>(std::distance(segPrefix_.begin(), it) -
                                          1); // bo upper_bound zwraca pierwszy elem większy od s
        float sLocal = s - segPrefix_[k];
        return std::make_pair(k, sLocal);
    }

    glm::vec3 Spline::getPositionAtS(float s) const {
        auto [k, sLocal] = locateSegmentByS(s);
        const auto& seg = lut_[k];

        if (sLocal <= 0.f)
            return getPosition(k, 0.f);
        if (sLocal >= seg.length)
            return getPosition(k, 1.f);

        const auto& samples = seg.samples;
        auto it1 = std::lower_bound(samples.begin(), samples.end(), sLocal,
                                    [](const ArcSample& a, float val) { return a.s < val; });

        auto it0 = std::prev(it1);
        float denom = std::max(it1->s - it0->s, 1e-6f);
        float alpha = (sLocal - it0->s) / denom;
        alpha = std::clamp(alpha, 0.f, 1.f);
        float uInit = it0->u + alpha * (it1->u - it0->u);

        float uRefined = refineUByNewton(k, uInit, sLocal, 2);

        return getPosition(k, uRefined);
    }

    glm::vec3 Spline::getTangentAtS(float s) const {
        auto [k, sLocal] = locateSegmentByS(s);
        const auto& seg = lut_[k];

        if (sLocal <= 0.f)
            return getTangent(k, 0.f);
        if (sLocal >= seg.length)
            return getTangent(k, 1.f);

        const auto& samples = seg.samples;
        auto it1 = std::lower_bound(samples.begin(), samples.end(), sLocal,
                                    [](const ArcSample& a, float val) { return a.s < val; });

        auto it0 = std::prev(it1);
        float denom = std::max(it1->s - it0->s, 1e-6f);
        float alpha = (sLocal - it0->s) / denom;
        alpha = std::clamp(alpha, 0.f, 1.f);
        float uInit = it0->u + alpha * (it1->u - it0->u);

        float uRefined = refineUByNewton(k, uInit, sLocal, 2);

        return getTangent(k, uRefined);
    }

    float Spline::refineUByNewton(std::size_t segmentIndex, float u0, float sLocal, int iterations) const {
        float u = u0;
        for (int iter = 0; iter < iterations; ++iter) {
            glm::vec3 deriv = getDerivative(segmentIndex, u);
            float speed = glm::length(deriv);
            if (speed < kEps)
                break;

            // długość od 0 do u
            float sApprox = 0.f;
            const auto& samples = lut_[segmentIndex].samples;
            auto it1 = std::lower_bound(samples.begin(), samples.end(), u,
                                        [](const ArcSample& a, float val) { return a.u < val; });
            if (it1 != samples.begin()) {
                auto itPrev = std::prev(it1);
                sApprox = itPrev->s + glm::length(getPosition(segmentIndex, u) - itPrev->pos);
            } else {
                sApprox = glm::length(getPosition(segmentIndex, u) - samples.front().pos);
            }
            float delta = sApprox - sLocal;
            u -= delta / speed;
            u = std::clamp(u, 0.f, 1.f);
        }
        return u;
    }
} // namespace rc::math
