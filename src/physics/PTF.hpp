//
// Created by maciej on 15.08.25.
//

// PTFBuilder.hpp
#ifndef PTFBUILDER_HPP
#define PTFBUILDER_HPP

#include <functional>
#include <glm/vec3.hpp>
#include <vector>
#include "common/TrackTypes.hpp"
#include "physics/PathSampler.hpp"

    /*constexpr float kEps         = 1e-6f;
    constexpr float kEpsVertical = 1e-8f;*/

    struct MetaCallbacks
    {
        std::function<bool(float)> isInStation;
        std::function<float(float)> stationEdgeFadeWeight;
        std::function<float(float)> manualRollAtS;
    };

    namespace PTF
    {
        std::vector<Frame> build(const PathSampler& sampler, float ds, glm::vec3 globalUp, const MetaCallbacks& cb);
        static inline glm::vec3 rotateAroundAxis(const glm::vec3& v, const glm::vec3& axis, float angle) {
            return glm::angleAxis(angle, axis) * v;
        }
    };

#endif
