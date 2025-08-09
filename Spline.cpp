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
    if (segmentCount() == 0) throw std::out_of_range("Spline::locateSegmentByS invalid segment index");
    if (isClosed())
    {
        s = fmod(s, totalLength_);
        if (s < 0.f) s += totalLength_;
    } else
    {
        s = std::clamp(s, 0.f, totalLength_);
    }
    auto k = std::distance(segPrefix_.begin(),(std::ranges::upper_bound(segPrefix_, s)-1));
    float sLocal = s - segPrefix_[k];

    return std::make_pair(k, sLocal);
}

glm::vec3 Spline::getPositionAtS(float s) const
{
    auto locSeg = locateSegmentByS(s);
    std::size_t k = locSeg.first;
    float sLocal = locSeg.second;
    auto u1 = std::lower_bound(lut_[k].samples.begin(), lut_[k].samples.end(), sLocal);
    auto u0 = u1;
    if (u1 != lut_[k].samples.begin()) u0 -= 1;

    float alpha;
    float epsilon = 10e-6f;
    if (u1->s - u0->s >= epsilon)
    {
        alpha = (sLocal - u0->s) / (u1->s - u0->s);
    } else
    {
        alpha = (sLocal - u0->s) / epsilon;
    }
    float u = u0->u + alpha * (u1->u - u0->u);
    glm::vec3 pos = u0->pos + alpha * (u1->pos - u0->pos);

    return getPosition(k, u);
}

