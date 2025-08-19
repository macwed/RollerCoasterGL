//
// Created by maciej on 17.08.25.
//

#ifndef TRACKTYPES_HPP
#define TRACKTYPES_HPP

#include <glm/vec3.hpp>

namespace rc::common
{
    enum  class EdgeType { CatmullRom, Linear, Circular, Helix };
    struct EdgeMeta { EdgeType type{ EdgeType::CatmullRom}; };
    struct NodeMeta
    {
        bool stationStart = false, stationEnd = false;
        float length = 0.f, userRoll = 0.f;
    };
    struct RollKey { float s = 0.f, roll = 0.f; };
    struct Frame { glm::vec3 pos, T, N, B; float s = 0.f; };
}

#endif //TRACKTYPES_HPP
