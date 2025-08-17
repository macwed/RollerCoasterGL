//
// Created by maciej on 15.08.25.
//

#include "PTFBuilder.hpp"
#include <algorithm>
#include <glm/geometric.hpp>
#include <glm/gtx/quaternion.hpp>

using namespace physics;

static inline glm::vec3 rotateAroundAxis(const glm::vec3& v, const glm::vec3& axis, float angle) {
    return glm::angleAxis(angle, axis) * v;
}

std::vector<Frame> PTFBuilder::build(const Spline& spline, float ds, glm::vec3 globalUp,
                                                    const MetaCallbacks& meta)
{
    const float trackLength = spline.totalLength();
    if (trackLength <= 0.f) return {};

    std::vector<Frame> frames;
    frames.reserve(static_cast<std::size_t>(trackLength / ds) + 2);

    glm::vec3 P0 = spline.getPositionAtS(0.f);
    glm::vec3 T0 = glm::normalize(spline.getTangentAtS(0.f));

    glm::vec3 N0_raw = globalUp -  T0 * glm::dot(globalUp, T0);
    if (glm::length2(N0_raw) < kEpsVertical) //zabezpieczenie gdyby jednak punkt startowy był (prawie) pionowy... pionowa stacja? cmon...
    {
        glm::vec3 tmp = (std::abs(T0.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3 (1, 0, 0));
        N0_raw = tmp - T0 * glm::dot(tmp, T0);
    }
    glm::vec3 N0 = glm::normalize(N0_raw);
    glm::vec3 B0 = glm::normalize(glm::cross(T0, N0));
    N0 = glm::normalize(glm::cross(B0, T0));
    frames.emplace_back(Frame{P0, T0, N0, B0, 0.f});

    //rotacja wektora styczna względem wektora stycznego w punkcie poprzednim i wyliczenie norm,binorm
    for (float s = ds; s < trackLength; s+=ds)
    {
        glm::vec3 P = spline.getPositionAtS(s);
        glm::vec3 T = spline.getTangentAtS(s);

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
            if (cos_phi < 0.f) {
                // T_curr ~ -T_prev
                N = -N_prev;
                B = -B;
            }
        }

        bool inStation = false;
        if (meta.isInStation) inStation = meta.isInStation(s);

        if (inStation || (meta.stationEdgeFadeWeight && meta.stationEdgeFadeWeight(s)) > 0.f)
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
            } else
            {
                float w = meta.stationEdgeFadeWeight ? meta.stationEdgeFadeWeight(s) : 0.f;
                if (w > 0.f) {
                    N = glm::normalize(glm::mix(N, Ng, w));
                    B = glm::normalize(glm::cross(T, N));
                    N = glm::normalize(glm::cross(B, T));
                }
            }
        }

        if (!inStation && meta.manualRollAtS)
        {
            float roll = meta.manualRollAtS(s);
            if (std::abs(roll) > kEps)
            {
                N = rotateAroundAxis(N, T, roll);
                B = glm::normalize(glm::cross(T, N));
                N = glm::normalize(glm::cross(B, T));
            }
        }
        frames.emplace_back(Frame{P, T, N, B, s});
    }
    glm::vec3 Pend = spline.getPositionAtS(trackLength);
    glm::vec3 Tend = glm::normalize(spline.getTangentAtS(trackLength));
    frames.emplace_back(Frame{
        Pend, Tend, frames.back().N, frames.back().B, trackLength});
    return frames;
}