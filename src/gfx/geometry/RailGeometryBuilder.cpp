//
// Created by maciej on 15.08.25.
//
#include "RailGeometryBuilder.hpp"

#include <cassert>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

namespace rc::gfx::geometry {
    bool RailGeometryBuilder::build(const RailParams& p) {
        vertices_.clear();
        indices_.clear();

        const bool closed = p.closedLoop;
        if (frames_.size() < 2 || p.ringSides < 3 || p.gauge <= kEps || p.railRadius <= kEps)
            return false;

        // Czy ostatnia ramka duplikuje pozycję pierwszej?
        const glm::vec3 dp = frames_.back().pos - frames_.front().pos;
        const bool hasDuplicateEnd = (glm::dot(dp, dp) < closeEps2);
        const bool closedEff = closed && hasDuplicateEnd;

        // Ile ringów faktycznie rysujemy i ile segmentów zszywamy
        uint32_t ringsTotal =
                closedEff ? static_cast<uint32_t>(frames_.size()) - 1u : static_cast<uint32_t>(frames_.size());
        uint32_t segs = closedEff ? ringsTotal : (ringsTotal - 1u);

        auto nextFrame = [ringsTotal, closedEff](uint32_t i) {
            return (closedEff && (i + 1u == ringsTotal)) ? 0u : (i + 1u);
        };

        const uint32_t ring = p.ringSides + 1u; // +1 bo duplikujemy pierwszy profil, by zamknąć okrąg
        constexpr uint32_t rails = 2u;

        const uint32_t vertsTotal = ringsTotal * ring * rails;
        const uint32_t quadsPerRailPerSeg = ring - 1u;
        const uint32_t trisTotal = segs * quadsPerRailPerSeg * rails * 2u;

        vertices_.resize(vertsTotal);
        indices_.resize(trisTotal * 3u);

        // Ringi – generujemy WSZYSTKIE r0..r_(ringsTotal-1)
        for (uint32_t i = 0; i < ringsTotal; ++i) {
            rings_(i, frames_[i].pos, frames_[i].N, frames_[i].B, p, ringsTotal, closedEff);
        }

        auto vidx = [ring](uint32_t frameIdx, uint32_t rail, uint32_t r) {
            return frameIdx * (ring * 2u) + (rail ? ring + r : r);
        };

        // Zszywanie
        size_t w = 0;
        for (uint32_t i = 0; i < segs; ++i) {
            const uint32_t j = nextFrame(i);
            for (uint32_t r = 0; r < ring - 1u; ++r) {
                const uint32_t rNext = r + 1u;

                // lewa
                uint32_t a = vidx(i, 0, r);
                uint32_t b = vidx(i, 0, rNext);
                uint32_t c = vidx(j, 0, r);
                uint32_t d = vidx(j, 0, rNext);
                indices_[w++] = a;
                indices_[w++] = b;
                indices_[w++] = c;
                indices_[w++] = b;
                indices_[w++] = c;
                indices_[w++] = d;

                // prawa
                a = vidx(i, 1, r);
                b = vidx(i, 1, rNext);
                c = vidx(j, 1, r);
                d = vidx(j, 1, rNext);
                indices_[w++] = a;
                indices_[w++] = b;
                indices_[w++] = c;
                indices_[w++] = b;
                indices_[w++] = c;
                indices_[w++] = d;
            }
        }
        assert(w == indices_.size());
        return true;
    }


    void RailGeometryBuilder::rings_(uint32_t frameIdx, const glm::vec3& centerPos, const glm::vec3& N,
                                     const glm::vec3& B, const RailParams& params, const uint32_t ringsTotal,
                                     const bool closedEff) {

        const auto ring = params.ringSides + 1;
        const auto gauge = params.gauge;
        const auto radius = params.railRadius;
        const bool useStartNB = params.closedLoop && (frameIdx + 1 == ringsTotal) && closedEff;

        const glm::vec3 centerL = centerPos + B * gauge * 0.5f;
        const glm::vec3 centerR = centerPos - B * gauge * 0.5f;

        size_t base = frameIdx * ring * 2;
        const glm::vec3& n = useStartNB ? frames_.front().N : N;
        const glm::vec3& b = useStartNB ? frames_.front().B : B;
        for (size_t i = 0; i < ring; ++i) {
            float u = (i == ring - 1) ? 1.0f : static_cast<float>(i) / static_cast<float>(ring - 1);
            float angle = twoPi * u;
            glm::vec3 circDir = glm::cos(angle) * b + glm::sin(angle) * n;
            glm::vec3 offset = circDir * radius;
            const float v = frames_[frameIdx].s * params.texScaleV;

            vertices_[i + base] = {centerL + offset, circDir, {u, v}};
            vertices_[i + base + ring] = {centerR + offset, circDir, {u, v}};
        }
    }
} // namespace rc::gfx::geometry
