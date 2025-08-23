//
// Created by maciej on 15.08.25.
//
#include "RailGeometryBuilder.hpp"

#include <glm/ext/quaternion_geometric.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>
#include <span>
#include <vector>

namespace rc::gfx::geometry {
bool RailGeometryBuilder::build(const RailParams& p) {

  const bool closed = p.closedLoop;
  vertices_.clear(); indices_.clear();

  if (frames_.size() < 2 || p.ringSides < 3 || p.gauge < kEps || p.railRadius < kEps) {
    //std::cerr << "Error: RailGeometryBuilder::build failed. Check RailParams and frames_" << std::endl;
    vertices_.clear();
    indices_.clear();
    return false;
  }

  bool hasDuplicateEnd = false;
  const glm::vec3 dp = frames_.back().pos - frames_.front().pos;
  hasDuplicateEnd = (glm::dot(dp,dp) < closeEps2);

  const bool closedEff = closed && hasDuplicateEnd;


  const uint32_t ring = p.ringSides + 1;
  uint32_t ringsTotal = static_cast<uint32_t>(frames_.size()) - (hasDuplicateEnd ? 1u : 0u);
  uint32_t segs = closedEff ? ringsTotal : (ringsTotal - 1);
  const uint32_t rails = 2;
  const uint32_t vertsTotal = ringsTotal * ring * rails;
  const uint32_t quadsPerRailPerSeg = ring - 1;
  const uint32_t trisTotal = segs * quadsPerRailPerSeg * rails * 2; //segment, prostokąt, lewy prawy *2, prostokąt to 2*trójkąt

  auto nextFrame = [&](uint32_t i) {
    return (closedEff && (i + 1 == ringsTotal)) ? 0u : (i + 1);
  };

  vertices_.resize(vertsTotal);
  indices_.resize(trisTotal * 3);

  for (size_t i = 0; i < ringsTotal; ++i) {
    rings_(i, frames_[i].pos, frames_[i].N, frames_[i].B, p, ringsTotal, closedEff);
  }

  auto vidx = [ring](uint32_t frameIdx, uint32_t rail, uint32_t r) -> uint32_t {
    //rail = 0 dla lewej szyny, rail = 1 dla prawej szyny
    return frameIdx * (ring * 2) + (rail ? ring + r : r);
  };

  std::size_t w = 0;
  for (uint32_t i = 0; i < segs; ++i) {
    const uint32_t j = nextFrame(i);
    for (uint32_t r = 0; r < ring - 1; ++r) {
      uint32_t rNext = r + 1;

      //lewa szyna
      uint32_t a = vidx(i,     0, r);
      uint32_t b = vidx(i,     0, rNext);
      uint32_t c = vidx(j, 0, r);
      uint32_t d = vidx(j, 0, rNext);
      //trójkąty abc bcd
      indices_[w++] = a; indices_[w++] = b; indices_[w++] = c;
      indices_[w++] = b; indices_[w++] = c; indices_[w++] = d;

      //prawa szyna
      a = vidx(i,     1, r);
      b = vidx(i,     1, rNext);
      c = vidx(j, 1, r);
      d = vidx(j, 1, rNext);
      indices_[w++] = a; indices_[w++] = b; indices_[w++] = c;
      indices_[w++] = b; indices_[w++] = c; indices_[w++] = d;
    }
  }
  assert(w == indices_.size());           // szybki sanity-check

  return true;
}

void RailGeometryBuilder::rings_(uint32_t frameIdx,
                                  const glm::vec3& centerPos,
                                  const glm::vec3& N,
                                  const glm::vec3& B,
                                  const RailParams& params,
                                  const uint32_t ringsTotal,
                                  const bool closedEff) {

  const auto ring = params.ringSides + 1;
  const auto gauge = params.gauge;
  const auto radius = params.railRadius;
  const bool useStartNB = params.closedLoop
                      && (frameIdx + 1 == ringsTotal) && closedEff;

  const glm::vec3 centerL = centerPos + B * gauge * 0.5f;
  const glm::vec3 centerR = centerPos - B * gauge * 0.5f;

  size_t base = frameIdx * ring * 2;
  const glm::vec3& n = useStartNB ? frames_.front().N : N;
  const glm::vec3& b = useStartNB ? frames_.front().B : B;
  for (size_t i = 0; i < ring; ++i) {
    float u = (i == ring - 1) ? 1.0f : static_cast<float> (i) / static_cast<float>(ring - 1);
    float angle = twoPi * u;
    glm::vec3 circDir   = glm::cos(angle) * b + glm::sin(angle) * n;
    glm::vec3 offset = circDir * radius;
    const float v = frames_[frameIdx].s * params.texScaleV;

    vertices_[i + base] = {centerL + offset, circDir, { u, v } };
    vertices_[i + base + ring] = { centerR + offset, circDir, { u, v } };
  }
}
}