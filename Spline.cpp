//
// Created by maciej on 07.08.25.
//
#include <vector>
#include <glm/vec3.hpp>
#include "Spline.hpp"

#include <glm/geometric.hpp>

void Spline::addPoint(const glm::vec3& point)
{
    points_.push_back(point);
}


glm::vec3 Spline::getPosition(std::size_t segmentIndex, float t) const
{
    if (segmentIndex + 3 >= points_.size())
    {
        throw std::out_of_range("Spline::getPosition invalid segment index");
    }

    const glm::vec3& P0 = points_[segmentIndex + 0];
    const glm::vec3& P1 = points_[segmentIndex + 1];
    const glm::vec3& P2 = points_[segmentIndex + 2];
    const glm::vec3& P3 = points_[segmentIndex + 3];

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
    const glm::vec3& P0 = points_[segmentIndex + 0];
    const glm::vec3& P1 = points_[segmentIndex + 1];
    const glm::vec3& P2 = points_[segmentIndex + 2];
    const glm::vec3& P3 = points_[segmentIndex + 3];

    float t2 = t * t;

    //pochodna po t
    glm::vec3 derivative = 0.5f * (
        (-P0 + P2) +
        (2.f * P0 - 5.0f * P1 + 4.0f * P2 - P3) * (2.0f * t) +
        (-P0 + 3.0f * P1 - 3.0f * P2 + P3) * (3.0f * t2));

    return glm::normalize(derivative);
}