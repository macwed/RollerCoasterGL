//
// Created by maciej on 02.08.25.
//

#ifndef TRACK_HPP
#define TRACK_HPP
#include <vector>
#include <glm/gtc/quaternion.hpp>
#include "gfx/render/DrawableMixin.hpp"
#include "math/Spline.hpp"
#include "common/TrackTypes.hpp"

constexpr float kEpsVertical = 1e-8f;

class Track : public DrawableMixin<Track>{

public:
    Track();
    [[nodiscard]] std::vector<Frame> buildFrames(const Spline& spline, float ds,
                                       glm::vec3 globalUp = {0,1,0}) const;
    static float approximateSForPoint(const Spline& spline, const glm::vec3& p, float s0, float s1, float ds);

    [[nodiscard]] float stationEdgeFadeWeight(float s) const;
    [[nodiscard]] bool isInStation(float s) const;
    [[nodiscard]] bool isNearStationEdge(float s) const;
    void buildStationIntervals(const Spline& spline, float sampleStep);

    void rebuildRollKeys(const Spline& spline, float sampleStep);
    [[nodiscard]] float manualRollAtS(const Spline& spline, float s) const;

    void syncMetaWithSpline(const Spline& spline);
    void rebuildMeta(const Spline& spline);

    void buildMeshFromFrames();
    void uploadToGPU();
    void draw() const;
    void releaseGL();

private:
    std::vector<NodeMeta> nodeMeta_;
    std::vector<EdgeMeta> edgeMeta_;
    std::vector<std::pair<float, float>> stationIntervals_;
    std::vector<RollKey> rollKeys_;
    std::vector<glm::vec3> points_;
    std::vector<std::uint32_t> indices_;
    unsigned vbo_, vao_, ibo_;

    const float feather = 0.75f;

    static glm::vec3 rotateAroundAxis(const glm::vec3& v, const glm::vec3& axis, float angle);

};



#endif //TRACK_HPP
