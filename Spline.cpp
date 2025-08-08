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

    return glm::vec3(1.0f, 0.0f, 0.0f);
}