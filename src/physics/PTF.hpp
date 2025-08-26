//
// Created by maciej on 15.08.25.
//

// PTFBuilder.hpp
#ifndef PTF_HPP
#define PTF_HPP

#include <functional>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <vector>

#include "common/TrackTypes.hpp"
#include "physics/PathSampler.hpp"

namespace rc::physics {
    struct MetaCallbacks {
        std::function<bool(float)> isInStation;
        std::function<float(float)> stationEdgeFadeWeight;
        std::function<float(float)> manualRollAtS;
    };

    std::vector<common::Frame> buildFrames(const PathSampler& sampler, float ds, glm::vec3 globalUp,
                                           const MetaCallbacks& cb);
    static inline glm::vec3 rotateAroundAxis(const glm::vec3& v, const glm::vec3& axis, float angle) {
        return glm::angleAxis(angle, axis) * v;
    }
} // namespace rc::physics

#endif
