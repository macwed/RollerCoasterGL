//
// Created by mwed on 26.08.2025.
//

#include "Car.hpp"
#include "TrackComponent.hpp"

namespace rc::gameplay {
    void Car::update(float dt, const TrackComponent& track) {
        s += v * dt;
        if (track.isClosed()) {
            float L = track.totalLength();
            s = std::fmod(s, L);
            if (s < 0) s += L;
        }
        auto f = track.frameAtS(s);
        pos = f.pos;
        orientation = glm::mat3(f.T, f.N, f.B);
    }
}