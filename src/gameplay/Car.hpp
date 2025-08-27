//
// Created by mwed on 26.08.2025.
//

#ifndef ROLLERCOASTERGL_CAR_HPP
#define ROLLERCOASTERGL_CAR_HPP
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "TrackComponent.hpp"

namespace rc::gameplay {
    class Car {
        public:
        Car() = default;

        void update(float dt, const TrackComponent& track);
        glm::vec3 getPos() const { return pos_; }
        glm::mat3 getOrientation() const { return orientation_; }

    private:
        glm::vec3 pos_{0.f};
        glm::mat3 orientation_{1.f};
        TrackComponent::FrameLookup lookup_;

        float s_ = 0.0f;
        float v_ = 15.0f;
    };
}


#endif // ROLLERCOASTERGL_CAR_HPP
