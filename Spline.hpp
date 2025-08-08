//
// Created by maciej on 07.08.25.
//

#ifndef SPLINE_HPP
#define SPLINE_HPP
#include <vector>
#include <glm/vec3.hpp>

struct Node {
    glm::vec3 pos;
    float roll = 0.f;
    float tension = 0.f;
    float continuity = 0.f;
};

struct ArcSample
{
    float u; //lokalny parametr z zakresu [0,1] dla łuku
    float s; //odległość od początku TEGO segmnetu toru do u
    glm::vec3 pos; //pozycja C(u)
};

struct SegmentLUT
{
    std::vector<ArcSample> samples;
    float length = 0.f; //całkowita długość segmentu
};

class Spline {
public:
    void addNode(const Node& node);
    void insertNode(std::size_t i, const Node& node);
    void moveNode(std::size_t i, const glm::vec3& newPos);
    void removeNode(std::size_t i);

    std::size_t segmentCount() const;

    glm::vec3 getPosition(std::size_t segmentIndex, float t) const;
    glm::vec3 getTangent(std::size_t segmentIndex, float t) const;

private:
    std::vector<Node> nodes_;
    bool closed = false;
};



#endif //SPLINE_HPP
