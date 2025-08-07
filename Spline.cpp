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
