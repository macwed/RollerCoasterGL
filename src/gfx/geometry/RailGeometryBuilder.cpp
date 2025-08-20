//
// Created by maciej on 15.08.25.
//
#include "RailGeometryBuilder.hpp"

#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <span>
#include <vector>

#include "gameplay/TrackComponent.hpp"

namespace rc::gfx::geometry {
constexpr float kEps = 1e-6f;
constexpr auto twoPi = glm::two_pi<float>();

RailGeometryBuilder::RailGeometryBuilder(const gameplay::TrackComponent& track)
  : frames_(track.frames()) {}

void RailGeometryBuilder::build(const RailParams& params) {
  std::vector<glm::vec3> vertices;
  const auto fSize = frames_.size();
  const uint32_t ringSides = params.ringSides;
  if (fSize < 2 || ringSides < 3 || params.gauge < kEps || params.railRadius < kEps) {
    std::cerr << "Error: RailGeometryBuilder::build failed. Check RailParams and frames_" << std::endl;
    vertices_.clear();
    indices_.clear();
    return;
  }
  const auto segments = static_cast<uint32_t>(fSize - 1);
  const uint32_t quadsPerRingStrip = ringSides;
  const std::size_t triCount = static_cast<std::size_t>(segments) * quadsPerRingStrip * 2 * 2; //segment, prostokąt, lewy prawy *2, prostokąt to 2*trójkąt
  indices_.clear();
  indices_.resize(triCount * 3);
  vertices.resize(fSize * ringSides * 2);

  for (uint32_t i = 0; i < fSize; ++i) {
    auto ring = rings_(frames_[i].pos, frames_[i].T, frames_[i].B, params);
    auto base = i * ringSides * 2;
    std::move(ring.begin(), ring.end(), vertices.begin() + base);
  }
  auto vidx = [ringSides](uint32_t frameIdx, uint32_t rail, uint32_t r) -> uint32_t {
    //rail = 0 dla lewej szyny, rail = 1 dla prawej szyny
    return frameIdx * (ringSides * 2) + (rail ? ringSides + r : r);
  };
  for (uint32_t i = 0; i + 1 < fSize; ++i) {
    for (uint32_t r = 0; r < ringSides; ++r) {
      uint32_t rNext = (r + 1) % ringSides;
      //lewa szyna
      uint32_t a = vidx(i,     0, r);
      uint32_t b = vidx(i,     0, rNext);
      uint32_t c = vidx(i + 1, 0, r);
      uint32_t d = vidx(i + 1, 0, rNext);
      //trójkąty abc bcd
      indices_.insert(indices_.end(), {a, b, c, b, c, d});

      //prawa szyna
      a = vidx(i,     1, r);
      b = vidx(i,     1, rNext);
      c = vidx(i + 1, 1, r);
      d = vidx(i + 1, 1, rNext);
      indices_.insert(indices_.end(), {a, b, c, b, c, d});
    }
  }
}

std::vector<glm::vec3> RailGeometryBuilder::rings_(const glm::vec3& centerPos,
    const glm::vec3& axis, const glm::vec3& B, const RailParams& params) {

  const auto ringSides = params.ringSides;
  const auto gauge = params.gauge;
  const auto radius = params.railRadius;
  if (ringSides < 3 || gauge < kEps || radius < kEps) {
    std::cerr << "Invalid RailParams!" << std::endl;
    return {};
  }
  std::vector<glm::vec3> rings(ringSides * 2);
  for (int i = 0; i < ringSides; ++i) {
    float angle = twoPi * static_cast<float> (i) / static_cast<float>(ringSides);
    auto ringQuat = glm::angleAxis(angle, axis);
    glm::vec3 center1 = centerPos + B * params.gauge * 0.5f;
    glm::vec3 center2 = centerPos - B * params.gauge * 0.5f;
    glm::vec3 ringPoint1 = center1 + (ringQuat * B) * params.railRadius;
    glm::vec3 ringPoint2 = center2 + (ringQuat * B) * params.railRadius;
    rings[i] = ringPoint1;
    rings[i + ringSides] = ringPoint2;
  }
  return rings;
}


}