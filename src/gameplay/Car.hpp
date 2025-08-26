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
        glm::vec3 getPos() const { return pos; }
        glm::mat3 getOrientation() const { return orientation; }

    private:
        glm::vec3 pos;
        glm::mat3 orientation;

        float s = 0.0f;
        float v = 10.0f;
    };
}


#endif // ROLLERCOASTERGL_CAR_HPP
