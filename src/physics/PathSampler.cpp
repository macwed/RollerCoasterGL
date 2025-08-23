//
// Created by maciej on 18.08.25.
//
#include "PathSampler.hpp"

#include <algorithm>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/gtx/norm.hpp>
#include "math/Spline.hpp"


namespace rc::physics {
constexpr float kEps = 1e-6f;
inline bool finite3(const glm::vec3& v)
{
  return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}


PathSampler::PathSampler(const math::Spline& spline, const std::vector<common::EdgeMeta>& e)
                            : spline_(spline), edges_(e) {}
Sample PathSampler::sampleAtS(float s) const {
  const float L = spline_.totalLength();
  s = spline_.isClosed()
        ? std::fmod(std::fmod(s, L) + L, L)
        : std::clamp(s, 0.f, L);

  if (spline_.segmentCount() == 0)
  {
    glm::vec3 p = (spline_.nodeCount() ? spline_.getNode(0).pos : glm::vec3(0.0f));
    return { p, glm::vec3(1.f, 0.f, 0.f) };
  }

  auto [seg, sLocal] = spline_.locateSegmentByS(s);

  if (seg < edges_.size() && edges_[seg].type == common::EdgeType::Linear)
  {
    glm::vec3 P1;
    glm::vec3 P2;
    if (spline_.isClosed()) {
      auto n = spline_.nodeCount();
      P1 = spline_.getNode((seg + 1) % n).pos;
      P2 = spline_.getNode((seg + 2) % n).pos;
    } else {
      P1 = spline_.getNode(seg + 1).pos;
      P2 = spline_.getNode(seg + 2).pos;
    }

    const float segLen = spline_.arcLengthAtSegmentEnd(seg) - spline_.arcLengthAtSegmentStart(seg);
    const float u = segLen > kEps ? std::clamp(sLocal / segLen, 0.0f, 1.0f) : 0.0f;

    const glm::vec3 pos = glm::mix(P1, P2, u);
    glm::vec3 dir = P2 - P1;
    glm::vec3 tan = (glm::length2(dir) > kEps) ? glm::normalize(dir) : glm::vec3(1.f, 0.f, 0.f);

    return { pos, tan };
  }

  glm::vec3 pos = spline_.getPositionAtS(s);
  glm::vec3 tan = spline_.getTangentAtS(s);

  if (!finite3(tan) || glm::length2(tan) < kEps)
  {
    const float ds = 1e-3f * std::max(1.0f, spline_.totalLength());
    const glm::vec3 p0 = spline_.getPositionAtS(s - ds);
    const glm::vec3 p1 = spline_.getPositionAtS(s + ds);
    const glm::vec3 d = p1 - p0;
    tan = (glm::length2(d) > kEps) ? glm::normalize(d) : glm::vec3(1.f, 0.f, 0.f);
  }
  return { pos, tan };
}
}