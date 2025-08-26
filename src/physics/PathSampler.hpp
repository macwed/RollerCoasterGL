//
// Created by maciej on 18.08.25.
//

#ifndef PATHSAMPLER_HPP
#define PATHSAMPLER_HPP
#include <glm/vec3.hpp>
#include <vector>

#include "common/TrackTypes.hpp"
#include "math/Spline.hpp"

namespace rc::physics {
    constexpr float kEps = 1e-6f;
    constexpr float kEps2 = kEps * kEps;

    struct Sample {
        glm::vec3 pos, tan;
    };

    class PathSampler {
    public:
        PathSampler(const math::Spline& spline, const std::vector<common::EdgeMeta>& e);

        [[nodiscard]] Sample sampleAtS(float s) const;
        [[nodiscard]] float totalLength() const {
            return spline_.totalLength();
        }
        [[nodiscard]] bool isClosed() const {
            return spline_.isClosed();
        }

    private:
        const math::Spline& spline_;
        const std::vector<common::EdgeMeta>& edges_;
    };
} // namespace rc::physics


#endif // PATHSAMPLER_HPP
