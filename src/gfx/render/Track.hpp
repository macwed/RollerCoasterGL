//
// Created by maciej on 02.08.25.
//

#ifndef TRACK_HPP
#define TRACK_HPP

#include <vector>
#include <glm/glm.hpp>
#include "gfx/render/Mesh.hpp"
#include "gfx/geometry/RailGeometryBuilder.hpp"
#include "gfx/geometry/SupportGeometryBuilder.hpp"
#include "common/TrackTypes.hpp"
#include "terrain/Terrain.hpp"

namespace rc::gfx::render {

    struct InfraParams {
        float beamDs       = 0.60f; // poprzeczka po łuku
        float beamThick    = 0.08f;
        float beamHeight   = 0.06f;
        float supportHoriz = 3.0f;  // słup co 3 m po ziemi (rzut poziomy)
        float supportRadius= 0.06f;
        int   supportSides = 12;
        float minClearance = 0.25f; // minimalna wysokość słupa
    };

    class Track {
    public:
        void build(const std::vector<common::Frame>& frames,
                   const geometry::RailParams& railParams,
                   const Terrain& terrain,
                   const InfraParams& infra);

        void uploadToGPU()   { steel_.uploadToGPU(); }
        void draw() const    { steel_.draw(); }
        void releaseGL()     { steel_.release(); }

    private:
        static float totalLength(const std::vector<common::Frame>& fr) {
            if (fr.empty()) return 0.f;
            return fr.back().s; // s - długość łuku
        }

        Mesh steel_; // szyny + poprzeczki + słupy -- jeden drawcall
    };

} // namespace rc::gfx::render

#endif // TRACK_HPP
