//
// Created by maciej on 07.08.25.
//

#ifndef SPLINE_HPP
#define SPLINE_HPP
#include <stdexcept>
#include <vector>
#include <glm/vec3.hpp>


class Spline {
public:
    void addPoint(const glm::vec3& point);

private:
    std::vector<glm::vec3> points_;
};



#endif //SPLINE_HPP
