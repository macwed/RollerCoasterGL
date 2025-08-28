//
// Created by mwed on 26.08.2025.
//

#ifndef ROLLERCOASTERGL_CAR_HPP
#define ROLLERCOASTERGL_CAR_HPP
#include <functional>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "physics/FrameCursor.hpp"
#include "TrackComponent.hpp"

namespace rc::gameplay {
    class Car {
        public:
        Car() = default;

        float m = 400.f; //masa
        float g = 9.81f;
        float muRoll = 0.002f; //tarcie toczne
        float kAir = 0.02f; // opór powietrza
        float vMax = 550.f;
        float vStopEps = 0.02f; //próg dla zatrzymania
        glm::vec3 up = {0.f, 1.f, 0.f};

        float s = 0.0f;
        float v = 15.0f;

        std::function <float(float s, float v)> extraAccel;
        void kick(float v0) {v = v0;}

        void bindTrack(const TrackComponent& track);
        void update(float dt, const TrackComponent& track);
        [[nodiscard]] glm::vec3 getPos() const { return pos_; }
        [[nodiscard]] glm::mat3 getOrientation() const { return orientation_; }

    private:
        physics::FrameCursor cursor_;
        std::size_t frameIdxCache_ = 0;
        glm::vec3 pos_{0.f};
        glm::mat3 orientation_{1.f};
        bool backwards_ = false;
        const float vOn = 0.12f;
        const float vOff = 0.08f;
    };
}


#endif // ROLLERCOASTERGL_CAR_HPP
