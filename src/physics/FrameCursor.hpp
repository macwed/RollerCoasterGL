//
// Created by mwed on 28.08.2025.
//

#ifndef ROLLERCOASTERGL_FRAMECURSOR_HPP
#define ROLLERCOASTERGL_FRAMECURSOR_HPP

#include <algorithm>
#include <cmath>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <vector>

#include "common/TrackTypes.hpp"

namespace rc::physics {
    class FrameCursor {
    public:
        FrameCursor() = default;
        FrameCursor(const std::vector<common::Frame>* frames, bool closed, float length)
            : F_(frames), closed_(closed), L_(length) {}

        void reset(const std::vector<common::Frame>* frames, bool closed, float length) {
            F_ = frames;
            closed_ = closed;
            L_ = length;
            i_ = 0;
        }

        void sample(float sQuery, glm::vec3& pos, glm::vec3& T, glm::vec3& N, glm::vec3& B, glm::quat& q);

    private:
        const std::vector<common::Frame>* F_ = nullptr;
        bool closed_ = false;
        float L_ = 0.f;
        std::size_t i_ = 0;

        static float wrap(float s, float L, bool closed) {
            return closed ? (std::fmod(std::fmod(s, L) + L, L)) : (std::clamp(s, 0.f, L));
        }
    };
} // namespace rc::physics


#endif // ROLLERCOASTERGL_FRAMECURSOR_HPP
