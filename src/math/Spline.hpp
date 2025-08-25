//
// Created by maciej on 07.08.25.
//

#ifndef SPLINE_HPP
#define SPLINE_HPP
#include <utility>
#include <vector>
#include <glm/vec3.hpp>

namespace rc::math {
constexpr float kEps = 1e-6f;

struct Node {
  glm::vec3 pos;
  float roll = 0.f;
  float tension = 0.f;
  float continuity = 0.f;
  float bias = 0.f;
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

  [[nodiscard]] bool isNodeOnCurve(std::size_t i) const;
  [[nodiscard]] float sAtNode(std::size_t i) const;
  [[nodiscard]] std::size_t segmentIndexEndingAtNode(std::size_t i) const;
  [[nodiscard]] std::size_t segmentIndexStartingAtNode(std::size_t i) const;

  [[nodiscard]] std::size_t segmentCount() const;
  void rebuildArcLengthLUT(std::size_t minSamplesPerSegment = 64);
  [[nodiscard]] float totalLength() const noexcept { return totalLength_; }


  [[nodiscard]] glm::vec3 getPosition(std::size_t segmentIndex, float t) const;
  [[nodiscard]] glm::vec3 getTangent(std::size_t segmentIndex, float t) const;

  [[nodiscard]] std::pair<std::size_t, float> locateSegmentByS(float s) const;
  [[nodiscard]] glm::vec3 getPositionAtS(float s) const;
  [[nodiscard]] glm::vec3 getTangentAtS(float s) const;

  [[nodiscard]] float arcLengthAtSegmentStart(std::size_t seg) const;
  [[nodiscard]] float arcLengthAtSegmentEnd(std::size_t seg) const;

  void setClosed(bool c) noexcept { closed_ = c; };
  [[nodiscard]] bool isClosed() const;
  [[nodiscard]] bool hasValidLUT() const {
    return !lut_.empty() && lut_.size() == segmentCount();
  }

  [[nodiscard]] Node getNode(std::size_t i) const
  {
    return nodes_[i];
  }

  [[nodiscard]] std::size_t nodeCount() const
  {
    return nodes_.size();
  }

private:
  std::vector<Node> nodes_;
  std::vector<SegmentLUT> lut_; //jeden LUT na segment
  std::vector<float> segPrefix_;
  bool closed_ = false;
  float totalLength_ = 0.f;

  [[nodiscard]] glm::vec3 getDerivative(std::size_t segmentIndex, float t) const;
  [[nodiscard]] float refineUByNewton(std::size_t segmentIndex, float u0, float sLocal, int iterations) const;
  [[nodiscard]] std::size_t wrap(std::size_t i, std::size_t n) const {
    return (i % n + n) % n;
  }
};
}


#endif //SPLINE_HPP
