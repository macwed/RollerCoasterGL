//
// Created by maciej on 15.08.25.
//

#include <algorithm>
#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>
#include "PTF.hpp"

#include <iostream>
#include <ostream>

namespace rc::physics {
constexpr float kEpsVertical = 1e-8f;
constexpr float kEps = 1e-6f;

std::vector<common::Frame> buildFrames(const PathSampler& sampler, float ds, glm::vec3 globalUp,
                                      const MetaCallbacks& cb)
{
  if (ds <= 0.f)
  {
    std::cerr << "Warning: ds <= 0. Set ds to 0.05" << std::endl;
    ds = 0.05f;
  }
  const float trackLength = sampler.totalLength();
  if (trackLength <= 0.f) return {};

  std::vector<common::Frame> frames;
  frames.reserve(static_cast<std::size_t>(trackLength / ds) + 2);

  glm::vec3 P0 = sampler.sampleAtS(0.f).pos;
  glm::vec3 T0 = glm::normalize(sampler.sampleAtS(0.f).tan);

  glm::vec3 N0_raw = globalUp -  T0 * glm::dot(globalUp, T0);
  if (glm::length2(N0_raw) < kEpsVertical) //zabezpieczenie gdyby jednak punkt startowy był (prawie) pionowy? pionowa stacja? cmon
  {
    glm::vec3 tmp = (std::abs(T0.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3 (1, 0, 0));
    N0_raw = tmp - T0 * glm::dot(tmp, T0);
  }
  glm::vec3 N0 = glm::normalize(N0_raw);
  glm::vec3 B0 = glm::normalize(glm::cross(T0, N0));
  N0 = glm::normalize(glm::cross(B0, T0));
  frames.emplace_back(common::Frame{P0, T0, N0, B0, 0.f});

  //rotacja wektora styczna względem wektora stycznego w punkcie poprzednim i wyliczenie norm,binorm
  const bool closed = sampler.isClosed();
  auto loopCond = closed ? trackLength : (trackLength + 0.5f * ds);
  for (float s = ds; s < loopCond; s+=ds)
  {
    const Sample smpl = sampler.sampleAtS(s);
    glm::vec3 P = smpl.pos;
    glm::vec3 T = glm::normalize(smpl.tan);

    glm::vec3 T_prev = frames.back().T;
    glm::vec3 N_prev = frames.back().N;

    glm::vec3 v = glm::cross(T_prev, T);
    float sin_phi = glm::length(v);
    float cos_phi = std::clamp(glm::dot(T_prev, T), -1.0f, 1.0f);

    glm::vec3 N = N_prev;
    glm::vec3 B = frames.back().B;

    if (sin_phi >= kEps)
    {
      float phi = std::atan2(sin_phi, cos_phi);
      glm::vec3 axis = v / sin_phi; //norma
      glm::vec3 N_rot = rotateAroundAxis(N_prev, axis, phi);
      B = glm::normalize(glm::cross(T, N_rot));
      N = glm::normalize(glm::cross(B, T));
    }
    else
    {
      //taki bezpiecznik gdyby jednak vec styczne były równoległe o przeciwnych zwrotach - czasem zjebie ramkę na spojeniach segmentów itp
      if (cos_phi < -0.9999f) {
        // T_curr ~ -T_prev
        N = -N;
        B = -B;
      }
    }

    bool inStation = false;
    if (cb.isInStation) inStation = cb.isInStation(s);

    float w = (!inStation && cb.stationEdgeFadeWeight) ? cb.stationEdgeFadeWeight(s) : 0.f;
    if (inStation || w > 0.f)
    {
      glm::vec3 Ng = globalUp - T * glm::dot(globalUp, T);
      if (glm::length2(Ng) <= kEpsVertical) {
        glm::vec3 fallback = (std::abs(T.y) < 0.9f) ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
        Ng = fallback - T * glm::dot(fallback, T);
      }
      Ng = glm::normalize(Ng);
      glm::vec3 Bg = glm::normalize(glm::cross(T, Ng));
      Ng = glm::normalize(glm::cross(Bg, T));

      if (inStation) {
        N = Ng;
        B = Bg;
      } else if (w > 0.f)
      {
        N = glm::normalize(glm::mix(N, Ng, w));
        B = glm::normalize(glm::cross(T, N));
        N = glm::normalize(glm::cross(B, T));
      }
    }

    if (!inStation && cb.manualRollAtS)
    {
      float roll = cb.manualRollAtS(s);
      if (std::abs(roll) > kEps)
      {
        N = rotateAroundAxis(N, T, roll);
        B = glm::normalize(glm::cross(T, N));
        N = glm::normalize(glm::cross(B, T));
      }
    }
    frames.emplace_back(common::Frame{P, T, N, B, s});
  }
  //ost ramka s = trackLength
  const Sample sl = sampler.sampleAtS(trackLength);
  glm::vec3 P_end = sl.pos;
  glm::vec3 T_end = glm::normalize(sl.tan);

  glm::vec3 T_prev = frames.back().T;
  glm::vec3 N_end  = frames.back().N;
  glm::vec3 B_end  = frames.back().B;

  glm::vec3 v = glm::cross(T_prev, T_end);
  float sin_phi = glm::length(v);
  float cos_phi = std::clamp(glm::dot(T_prev, T_end), -1.0f, 1.0f);
  if (sin_phi >= kEps) {
    float phi = std::atan2(sin_phi, cos_phi);
    glm::vec3 axis = v / sin_phi;
    N_end = rotateAroundAxis(N_end, axis, phi);
    B_end = glm::normalize(glm::cross(T_end, N_end));
    N_end = glm::normalize(glm::cross(B_end, T_end));
  } else if (cos_phi < -0.9999f) {
    N_end = -N_end; B_end = -B_end;
  }

  frames.emplace_back(common::Frame{ P_end, T_end, N_end, B_end, trackLength });
  if (closed) {
    frames.back().N = frames.front().N;
    frames.back().B = frames.front().B;
  }
  return frames;
}
}