//
// Created by maciej on 02.08.25.
//

#include "Track.hpp"

#include "physics/FrameCursor.hpp"

#include <glad.h>
#include <glm/glm.hpp>
#include <span>

#include "Track.hpp"
#include <glm/gtx/norm.hpp>
#include <glm/gtx/compatibility.hpp>

namespace rc::gfx::render {
    using geometry::MeshOut;
    using geometry::RailGeometryBuilder;
    using geometry::SupportGeometryBuilder;

    void Track::build(const std::vector<common::Frame>& frames,
                      const geometry::RailParams& railParams,
                      const Terrain& terrain,
                      const InfraParams& infra) {
        MeshOut out;

        // Szyny
        RailGeometryBuilder rgb(frames, out);
        rgb.build(railParams);

        //sampler ramek
        physics::FrameCursor cursor(&frames, railParams.closedLoop, totalLength(frames));

        //Poprzeczki co beamDs po łuku
        SupportGeometryBuilder sgb(out);
        const float gauge = railParams.gauge;

        const float sMax = totalLength(frames);
        for (float s = 0.0f; s < sMax - 1e-4f; s += infra.beamDs) {
            glm::vec3 P, T, N, B; glm::quat q;
            cursor.sample(s, P, T, N, B, q);
            glm::vec3 L = P + B * (gauge*0.5f);
            glm::vec3 R = P - B * (gauge*0.5f);
            sgb.addBeamBox(L, R, infra.beamThick, infra.beamHeight);
        }

        // Słupy co supportHoriz po ziemi! Nie po łuku
        const glm::vec3 UP(0,1,0);
        for (float s = 0.0f; s < sMax - 1e-4f; ) {
            glm::vec3 P, T, N, B; glm::quat q;
            cursor.sample(s, P, T, N, B, q);
            float Ty = glm::dot(T, UP);
            float cosPhi = std::sqrt(glm::max(0.0f, 1.0f - Ty*Ty));
            float ds = infra.supportHoriz / glm::max(cosPhi, 0.05f); // clamp przy prawie pionie

            float groundY = terrain.sampleHeightBilinear(P.x, P.z);
            if (P.y - groundY > infra.minClearance) {
                glm::vec3 bottom(P.x, groundY, P.z);
                sgb.addSupportCylinder(P, bottom, infra.supportRadius, infra.supportSides);
                // stopka
                sgb.addFootDisk(bottom, infra.supportRadius*2.2f, 12);
            }
            s += ds;
        }

        // Upload do GPU
        Mesh steel;
        steel.setData(out.vertices, out.indices);
        steel_.release();
        steel_ = std::move(steel);
        steel_.uploadToGPU();
    }
} // namespace rc::gfx::render
