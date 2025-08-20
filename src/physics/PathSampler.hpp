//
// Created by maciej on 18.08.25.
//

#ifndef PATHSAMPLER_HPP
#define PATHSAMPLER_HPP
#include <glm/vec3.hpp>
#include <vector>
#include "common/TrackTypes.hpp"
#include "math/Spline.hpp"

namespace rc::physics {
struct Sample
{
  glm::vec3 pos, tan;
};

class PathSampler {
public:
  PathSampler(const math::Spline& spline, const std::vector<common::EdgeMeta>& e);

  [[nodiscard]] Sample sampleAtS(float s) const;
  float totalLength() const { return spline_.totalLength(); }
private:
  const math::Spline& spline_;
  const std::vector<common::EdgeMeta>& edges_;
};
}


#endif //PATHSAMPLER_HPP
