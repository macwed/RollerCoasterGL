//
// Created by mwed on 28.08.2025.
//

#include "FrameCursor.hpp"

#include <glm/geometric.hpp>

namespace rc::physics {
    void FrameCursor::sample(float sQuery, glm::vec3& pos, glm::vec3& T, glm::vec3& N, glm::vec3& B) {
        if (!F_ || F_ -> empty()) {
            pos = {};
            T = {1.f, 0.f, 0.f};
            N = {0.f, 1.f, 0.f};
            B = {0.f, 0.f, 1.f};
            return;
        }

        float s = wrap(sQuery, L_, closed_);
        const auto& frames = *F_;
        i_ = std::min(i_, frames.size() - 1);

        while (i_ + 1 < frames.size() && s > frames[i_ + 1].s) ++i_;
        while (i_ > 0                 && s < frames[i_].s    ) --i_;

        const auto& a = frames[i_];
        const auto& b = (i_ + 1 < frames.size()) ? frames[i_ + 1] : frames.front();

        float denom = std::max(b.s - a.s, 1e-6f);
        float t = std::clamp((s - a.s) / denom, 0.f, 1.f);

        glm::quat qa = a.q;
        glm::quat qb = b.q;
        if (glm::dot(qa, qb) < 0.f) qb = -qb;
        glm::quat q = glm::normalize(glm::slerp(qa, qb, t));
        glm::mat3 R = glm::mat3_cast(q);

        pos = glm::mix(a.pos, b.pos, t);
        T   = R[0];
        N   = R[1];
        B   = R[2];
    }

} // namespace rc::physics

