//
// Created by mwed on 26.08.2025.
//

#include <cassert>
#include "Car.hpp"
#include "TrackComponent.hpp"

namespace rc::gameplay {
    void Car::bindTrack(const TrackComponent& track) {
        const auto& F = track.frames();
        assert(!F.empty());
        cursor_.reset(&F, track.isClosed(), track.totalLength());
        s = 0.0f;
    }

    void Car::update(float dt, const TrackComponent& track) {
        if (track.frames().empty()) return;

        constexpr float h = 1.0f / 240.0f;
        float tLeft = dt;

        glm::vec3 P, T, N, B;

        while (tLeft > 0.0f) {
            float dtSub = std::min(tLeft, h);

            cursor_.sample(s, P, T, N, B);

            float a_g = -g * glm::dot(up, T);
            float a_ext = extraAccel ? extraAccel(s, v) : 0.f;
            float a_air = -kAir * v * std::abs(v);
            float a_roll = 0.0f;
            if (std::abs(v) > 1e-4f) a_roll = -muRoll * g * static_cast<float>((v > 0) - (v < 0));
            else {
                float adr = a_g + a_ext;
                if (std::abs(adr) <= muRoll * g) a_roll = -adr;
                else a_roll = -muRoll * g * static_cast<float>((adr > 0) - (adr < 0));
            }
            float a = a_g + a_ext + a_air + a_roll;

            v += a * dtSub;
            v = std::clamp(v, -vMax, vMax);
            if (std::abs(v) < vStopEps && std::abs(a_g + a_ext) < muRoll * g) v = 0.0f;
            s += v * dtSub;

            //wrap lub odbicie
            float L = track.totalLength();
            if (track.isClosed()) s = std::fmod(std::fmod(s, L) + L, L);
            else {
                if (s < 0.f) {
                    s = 0.f;
                    v = 0.f;
                }
                if (s > L) {
                    s = L;
                    v = 0.f;
                }
            }
            tLeft -= dtSub;
        }
        //orientacja, odwrócenie T przy jeździe do tyłu
        cursor_.sample(s, P, T, N, B);
        glm::vec3 Tf = (v > 0) ? T : -T;
        glm::vec3 Bf = glm::normalize(glm::cross(Tf, N));
        N            = glm::normalize(glm::cross(Bf, Tf));

        pos_ = P;
        orientation_ = glm::mat3(Tf, N, Bf);
    }
}