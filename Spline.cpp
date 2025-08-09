//
// Created by maciej on 07.08.25.
//
#include <stdexcept>
#include <vector>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include "Spline.hpp"

#include <algorithm>

void Spline::addNode(const Node& node)
{
    nodes_.push_back(node);
}
void Spline::insertNode(std::size_t i, const Node& node)
{
    if (i > nodes_.size()) throw std::out_of_range("Spline::insertNode index out of range");
    nodes_.insert(nodes_.begin() + static_cast<ptrdiff_t>(i), node);
}
void Spline::moveNode(std::size_t i, const glm::vec3& newPos)
{
    if (i >= nodes_.size()) throw std::out_of_range("Spline::moveNode index out of range");
    nodes_[i].pos = newPos;
}
void Spline::removeNode(std::size_t i)
{
    if (i >= nodes_.size()) throw std::out_of_range("Spline::removeNode index out of range");
    nodes_.erase(nodes_.begin() + static_cast<ptrdiff_t>(i));
}

std::size_t Spline::segmentCount() const
{
    return nodes_.size() >= 4 ? nodes_.size() - 3 : 0;
}

glm::vec3 Spline::getPosition(std::size_t segmentIndex, float t) const
{
    if (segmentCount() == 0 || segmentIndex >= segmentCount())
    {
        throw std::out_of_range("Spline::getPosition invalid segment index");
    }

    const glm::vec3& P0 = nodes_[segmentIndex + 0].pos;
    const glm::vec3& P1 = nodes_[segmentIndex + 1].pos;
    const glm::vec3& P2 = nodes_[segmentIndex + 2].pos;
    const glm::vec3& P3 = nodes_[segmentIndex + 3].pos;

    t = std::clamp(t, 0.0f, 1.0f);
    float t2 = t * t;
    float t3 = t2 * t;

    /*Catmull-Rom równanie
    *C(t) = 0.5 * [(2P1) + (-P0 + P2)t + (2P0-5P1+4P2-P3)t^2 + (-P0+3P1-3P2+P3)t^3]
    *0.5 - tension
    *t - parametr [0,1] w obrębie segmentu między P1 i P2
    */

    return 0.5f * (
        (2.0f * P1) +
        (-P0 + P2) * t +
        (2.0f * P0 - 5.0f * P1 + 4.0f * P2 - P3) * t2 +
        (-P0 + 3.0f * P1 - 3.0f * P2 + P3) * t3
        );
}

glm::vec3 Spline::getTangent(std::size_t segmentIndex, float t) const
{
    if (segmentCount() == 0 || segmentIndex >= segmentCount())
    {
        throw std::out_of_range("Spline::getTangent invalid segment index");
    }

    const glm::vec3& P0 = nodes_[segmentIndex + 0].pos;
    const glm::vec3& P1 = nodes_[segmentIndex + 1].pos;
    const glm::vec3& P2 = nodes_[segmentIndex + 2].pos;
    const glm::vec3& P3 = nodes_[segmentIndex + 3].pos;

    t = std::clamp(t, 0.0f, 1.0f);
    float t2 = t * t;

    //pochodna po t
    glm::vec3 derivative = 0.5f * (
        (-P0 + P2) +
        (2.f * P0 - 5.0f * P1 + 4.0f * P2 - P3) * (2.0f * t) +
        (-P0 + 3.0f * P1 - 3.0f * P2 + P3) * (3.0f * t2)
        );

    constexpr float epsilon = 0.000001f;
    float len = glm::length(derivative);
    if (len >= epsilon)
    {
        return derivative / len;
    }

    glm::vec3 seg = P2 - P1;
    float segLen = glm::length(seg);
    if (segLen >= epsilon)
    {
        return seg / segLen;
    }

    glm::vec3 wide = P3 - P0;
    float wideLen = glm::length(wide);
    if (wideLen >= epsilon)
    {
        return wide / wideLen;
    }

    return {1.0f, 0.0f, 0.0f};
}

void Spline::rebuildArcLengthLUT(std::size_t minSamplesPerSegment)
{
    const auto segCount = segmentCount();
    if (segCount == 0)
    {
        lut_.clear();
        segPrefix_.clear();
        totalLength_ = 0.f;
        return;
    }

    if (minSamplesPerSegment < 2) minSamplesPerSegment = 2;
    lut_.clear();
    lut_.reserve(segCount);
    segPrefix_.assign(segCount, 0.f);
    constexpr float epsilon = 10e-6f;
    for (std::size_t seg = 0; seg < segCount; ++seg)
    {
        SegmentLUT segLUT;
        segLUT.samples.resize(minSamplesPerSegment + 1);
        glm::vec3 prevPos = getPosition(seg, 0);
        float s = 0.f;
        segLUT.samples[0] = {0.f, 0.f, prevPos};
        for (std::size_t i = 1; i <= minSamplesPerSegment; ++i)
        {
            float u = static_cast<float>(i) / static_cast<float>(minSamplesPerSegment);
            glm::vec3 pos = getPosition(seg, u);
            float ds = glm::length(pos - prevPos);
            if (ds < epsilon) ds = 0.f;
            s += ds;
            segLUT.samples[i] = {u, s, pos};
            prevPos = pos;
        }
        segLUT.length = s;
        lut_.push_back(std::move(segLUT));
    }

    for (std::size_t i = 1; i < segPrefix_.size(); ++i)
    {
        segPrefix_[i] = segPrefix_[i - 1] + lut_[i - 1].length;
    }
    totalLength_ = segPrefix_.back() + lut_.back().length;
}

bool Spline::isClosed() const
{
    if (segmentCount() == 0) return false;
    return glm::length(nodes_.front().pos - nodes_.back().pos) < 10e-4;
}

std::pair<std::size_t, float> Spline::locateSegmentByS(float s) const
{
    if (segmentCount() == 0) throw std::out_of_range("Spline::locateSegmentByS no segments");
    if (totalLength_ <= 0) return std::make_pair(0, 0.f);
    if (isClosed())
    {
        s = fmod(s, totalLength_);
        if (s < 0.f) s += totalLength_;
    } else
    {
        s = std::clamp(s, 0.f, totalLength_);
    }
    auto it = std::ranges::upper_bound(segPrefix_, s);
    if (it == segPrefix_.begin()) //s == 0
    {
        return std::make_pair(0, s);
    }
    if (it == segPrefix_.end()) //s == totalLength_
        {
            std::size_t k = segPrefix_.size() - 1;
            return std::make_pair(k, s - segPrefix_[k]);
        }
    std::size_t k = static_cast<std::size_t>(std::distance(segPrefix_.begin(), it));
    float sLocal = s - segPrefix_[k];
    return std::make_pair(k, sLocal);
}

glm::vec3 Spline::getPositionAtS(float s) const
{
    auto [k, sLocal] = locateSegmentByS(s);
    const auto& samples = lut_[k].samples;

    auto it1 = std::lower_bound(samples.begin(), samples.end(), sLocal,
                                [](const ArcSample& a, float val) { return a.s < val; });
    if (it1 == samples.end()) //sLocal na końcu segmentu
    {
        auto it0 = it1;
        it1 -= 1;
        it0 -= 2;
        float denom = std::max(it1->s - it0->s, 1e-6f);
        float alpha = (sLocal - it0->s) / denom;
        alpha = std::clamp(alpha, 0.f, 1.f); //ok ~1
        float u = it0->u + alpha * (it1->u - it0->u);  //ok ~1

        return getPosition(k, u);
    } else if (it1 == samples.begin())
    {
        auto it0 = it1;
        ++it1;
        float denom = std::max(it1->s - it0->s, 1e-6f);
        float alpha = (sLocal - it0->s) / denom;
        float u = it0->u + alpha * (it1->u - it0->u);

        return getPosition(k, u);
    } else
    {
        auto it0 = std::prev(it1);
        float denom = std::max(it1->s - it0->s, 1e-6f);
        float alpha = (sLocal - it0->s) / denom;
        alpha = std::clamp(alpha, 0.f, 1.f);
        float u = it0->u + alpha * (it1->u - it0->u);

        return getPosition(k, u);
    }
}