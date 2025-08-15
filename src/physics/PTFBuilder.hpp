//
// Created by maciej on 15.08.25.
//

// PTFBuilder.hpp
#ifndef PTFBUILDER_HPP
#define PTFBUILDER_HPP

#include <functional>
#include <glm/vec3.hpp>
#include <vector>
#include "math/Spline.hpp"

struct Frame {
    glm::vec3 pos, T, N, B;
    float s;
};

namespace physics {

    constexpr float kEps        = 1e-6f;
    constexpr float kEpsVertical= 1e-8f;

    struct MetaCallbacks {
        // Wszystkie funkcje są opcjonalne; sprawdzamy przed użyciem.
        // Uwaga: manualRollAtS potrzebuje Spline tylko do wrapowania – przekażemy go przez capture.
        std::function<bool(float)> isInStation;
        std::function<float(float)> stationEdgeFadeWeight;
        std::function<float(float)> manualRollAtS;
    };

    class PTFBuilder {
    public:
        static std::vector<Frame>
        build(const Spline& spline, float ds, glm::vec3 globalUp, const MetaCallbacks& meta);
    };

}

#endif
