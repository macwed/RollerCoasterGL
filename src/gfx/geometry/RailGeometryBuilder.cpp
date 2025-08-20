//
// Created by maciej on 15.08.25.
//
#include "RailGeometryBuilder.hpp"

#include <glm/ext/quaternion_geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>
#include <span>
#include <vector>

namespace rc::gfx::geometry {
constexpr float kEps = 1e-6f;
constexpr auto twoPi = glm::two_pi<float>();

void RailGeometryBuilder::build(const RailParams& p) {

  vertices_.clear(); indices_.clear();

  if (frames_.size() < 2 || p.ringSides < 3 || p.gauge < kEps || p.railRadius < kEps) {
    std::cerr << "Error: RailGeometryBuilder::build failed. Check RailParams and frames_" << std::endl;
    vertices_.clear();
    indices_.clear();
    return;
  }

  const uint32_t ring = p.ringSides;
  const uint32_t ringsTotal = static_cast<uint32_t>(frames_.size());
  const uint32_t segs = ringsTotal - 1; // wrap gdyby jednak closed loop
  const uint32_t rails = 2;
  const uint32_t vertsTotal = ringsTotal * ring * rails;
  const uint32_t quadsPerRailPerSeg = ring;
  const uint32_t trisTotal = segs * quadsPerRailPerSeg * rails * 2; //segment, prostokąt, lewy prawy *2, prostokąt to 2*trójkąt
  vertices_.resize(vertsTotal);
  indices_.resize(trisTotal * 3);

  for (uint32_t i = 0; i < ringsTotal; ++i) {
    rings_(i, frames_[i].pos, frames_[i].N, frames_[i].B, p);
  }

  auto vidx = [ring](uint32_t frameIdx, uint32_t rail, uint32_t r) -> uint32_t {
    //rail = 0 dla lewej szyny, rail = 1 dla prawej szyny
    return frameIdx * (ring * 2) + (rail ? ring + r : r);
  };

  std::size_t w = 0;
  for (uint32_t i = 0; i + 1 < ringsTotal; ++i) {
    for (uint32_t r = 0; r < ring; ++r) {
      uint32_t rNext = (r + 1) % ring;

      //lewa szyna
      uint32_t a = vidx(i,     0, r);
      uint32_t b = vidx(i,     0, rNext);
      uint32_t c = vidx(i + 1, 0, r);
      uint32_t d = vidx(i + 1, 0, rNext);
      //trójkąty abc bcd
      indices_[w++] = a; indices_[w++] = b; indices_[w++] = c;
      indices_[w++] = b; indices_[w++] = c; indices_[w++] = d;

      //prawa szyna
      a = vidx(i,     1, r);
      b = vidx(i,     1, rNext);
      c = vidx(i + 1, 1, r);
      d = vidx(i + 1, 1, rNext);
      indices_[w++] = a; indices_[w++] = b; indices_[w++] = c;
      indices_[w++] = b; indices_[w++] = c; indices_[w++] = d;
    }
  }
}

void RailGeometryBuilder::rings_(uint32_t frameIdx,
                                  const glm::vec3& centerPos,
                                  const glm::vec3& N,
                                  const glm::vec3& B,
                                  const RailParams& params) {

  const auto ringSides = params.ringSides;
  const auto gauge = params.gauge;
  const auto radius = params.railRadius;
  if (ringSides < 3 || gauge < kEps || radius < kEps) {
    std::cerr << "Invalid RailParams!" << std::endl;
    return;
  }
  const glm::vec3 centerL = centerPos + B * gauge * 0.5f;
  const glm::vec3 centerR = centerPos - B * gauge * 0.5f;

  uint32_t base = frameIdx * ringSides * 2;

  for (uint32_t i = 0; i < ringSides; ++i) {
    float angle = twoPi * static_cast<float> (i) / static_cast<float>(ringSides);
    glm::vec3 circDir   = glm::cos(angle) * B + glm::sin(angle) * N;
    glm::vec3 offset = circDir * radius;
    const float u = static_cast<float> (i) / static_cast<float>(ringSides);
    const float v = static_cast<float> (frameIdx);

    vertices_[i + base] = {centerL + offset, glm::normalize(circDir), { u, v } };
    vertices_[i + base + ringSides] = { centerR + offset, glm::normalize(circDir), { u, v } };
  }
}
}