//
// Created by maciej on 15.08.25.
//

#ifndef RAILGEOMETRYBUILDER_HPP
#define RAILGEOMETRYBUILDER_HPP
#include <glm/gtc/constants.hpp>
#include <glm/vec2.hpp>
#include <span>
#include <vector>

#include "common/TrackTypes.hpp"

namespace rc::gfx::geometry {
    constexpr float kEps = 1e-6f;
    constexpr auto twoPi = glm::two_pi<float>();
    constexpr float closeEps2 = 1e-6f;

    struct RailParams {
        float gauge = 1.1f; // odstęp szyn od osi czyli krzywej CR
        float railRadius = 0.12f; // średnica szyny
        unsigned ringSides = 10;
        bool closedLoop = false;
        float texScaleV = 1.0f;
    };

    struct Vertex {
        glm::vec3 pos, normal;
        glm::vec2 uv;
    };

    class RailGeometryBuilder {
    public:
        explicit RailGeometryBuilder(std::span<const common::Frame> frames) : frames_(frames){};

        bool build(const RailParams& p);

        [[nodiscard]] std::span<const Vertex> vertices() const {
            return vertices_;
        }
        [[nodiscard]] std::span<const uint32_t> indices() const {
            return indices_;
        }

    private:
        std::span<const common::Frame> frames_;
        std::vector<Vertex> vertices_; //(pos, normal, uv)
        std::vector<uint32_t> indices_;
        void rings_(uint32_t frameIdx, const glm::vec3& centerPos, const glm::vec3& N, const glm::vec3& B,
                    const RailParams& params, uint32_t ringsTotal, bool closedEff);
    };
} // namespace rc::gfx::geometry


#endif // RAILGEOMETRYBUILDER_HPP
