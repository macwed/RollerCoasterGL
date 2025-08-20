//
// Created by maciej on 15.08.25.
//

#ifndef RAILGEOMETRYBUILDER_HPP
#define RAILGEOMETRYBUILDER_HPP
#include <span>

#include "common/TrackTypes.hpp"
#include "gameplay/TrackComponent.hpp"

namespace rc::gfx::geometry {
struct RailParams {
  float gauge = 1.1f;        // odstęp szyn od osi czyli krzywej CR
  float railRadius = 0.12f; // średnica szyny
  int ringSides = 12;
  bool closeLoop = false;
};

class RailGeometryBuilder {
  public:
  RailGeometryBuilder(const gameplay::TrackComponent& track);
  void build(const RailParams& params);
private:
  std::span<const common::Frame> frames_;
  std::vector<glm::vec3> vertices_;
  std::vector<uint32_t> indices_;
  std::vector<glm::vec3> rings_(const glm::vec3& centerPos,
    const glm::vec3& axis, const glm::vec3& B, const RailParams& params);
};
}


#endif //RAILGEOMETRYBUILDER_HPP
