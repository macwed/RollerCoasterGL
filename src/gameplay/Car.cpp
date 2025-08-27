//
// Created by mwed on 26.08.2025.
//

#include "Car.hpp"
#include "TrackComponent.hpp"

namespace rc::gameplay {
    void Car::update(float dt, const TrackComponent& track) {
        s_ += v_ * dt;
        if (track.isClosed()) {
            float L = track.totalLength();
            s_ = std::fmod(s_, L);
            if (s_ < 0) s_ += L;
        }
        auto f = track.frameAtS_fast(s_, lookup_);
        pos_ = f.pos;
        orientation_ = glm::mat3(f.T, f.N, f.B);
    }
}