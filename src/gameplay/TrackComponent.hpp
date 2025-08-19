//
// Created by maciej on 18.08.25.
//

#ifndef TRACKCOMPONENT_HPP
#define TRACKCOMPONENT_HPP
#include <glm/vec3.hpp>
#include "common/TrackTypes.hpp"
#include "math/Spline.hpp"
#include "physics/PathSampler.hpp"

namespace rc::gameplay
{
    class TrackComponent {
    public:
      math::Spline& spline() { return spline_; }
        [[nodiscard]] const math::Spline& spline() const { return spline_; }
        std::vector<common::EdgeMeta>& edges() { return edgeMeta_; }
        const std::vector<common::Frame>& frames() { return frames_; }

        void markDirty() { dirtySpline_ = dirtyMeta_ = dirtyFrames_ = true; }
        void rebuild();

        [[nodiscard]] bool isInStation(float s) const;
        [[nodiscard]] float stationEdgeFadeWeight(float s) const;
        [[nodiscard]] float manualRollAtS(float s) const;
        [[nodiscard]] glm::vec3 positionAtS(float s) const;
        [[nodiscard]] glm::vec3 tangentAtS(float s) const;
        [[nodiscard]] common::Frame frameAtS(float s) const;

        void setDs(float v) { ds_ = v; dirtyFrames_ = true;}
        void setUp(glm::vec3 up) { up_ = up; dirtyFrames_ = true;}

    private:
        math::Spline spline_;
        std::vector<common::EdgeMeta> edgeMeta_;
        std::vector<common::NodeMeta> nodeMeta_;
        std::vector<std::pair<float, float>> stations_;
        std::vector<common::RollKey> rollKeys_;
        std::vector<common::Frame> frames_;
        float ds_ = 0.1f;
        const float feather_ = 0.75f;
        glm::vec3 up_{0.0f, 1.0f, 0.0f};
        bool dirtySpline_ = true, dirtyMeta_ = true, dirtyFrames_ = true;

        static float approximateSForPoint_(const math::Spline& spline, const glm::vec3& p, float s0, float s1, float ds);
        void rebuildLUT_();
        void syncMetaWithSpline_();
        void buildStationIntervals_();
        void rebuildRollKeys_();
        void buildFrames_();
    };
}



#endif //TRACKCOMPONENT_HPP
