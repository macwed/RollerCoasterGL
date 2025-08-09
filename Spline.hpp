//
// Created by maciej on 07.08.25.
//

#ifndef SPLINE_HPP
#define SPLINE_HPP
#include <utility>
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
    glm::vec3 pos; //pozycja C(u);
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

    [[nodiscard]] std::size_t segmentCount() const;
    void rebuildArcLengthLUT(std::size_t minSamplesPerSegment = 64);
    [[nodiscard]] float totalLength() const noexcept { return totalLength_; }

    [[nodiscard]] glm::vec3 getPosition(std::size_t segmentIndex, float t) const;
    [[nodiscard]] glm::vec3 getTangent(std::size_t segmentIndex, float t) const;

    glm::vec3 getPositionAtS(float s) const;
    glm::vec3 getTangentAtS(float s) const;

    bool isClosed() const;

private:
    std::vector<Node> nodes_;
    std::vector<SegmentLUT> lut_; //jeden LUT na segment
    std::vector<float> segPrefix_;
    //bool closed_ = false;
    float totalLength_ = 0.f;

    [[nodiscard]] std::pair<std::size_t, float> locateSegmentByS(float s) const;
};



#endif //SPLINE_HPP
