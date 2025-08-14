//
// Created by maciej on 02.08.25.
//

#ifndef TRACK_HPP
#define TRACK_HPP
#include <vector>
#include <glm/gtc/quaternion.hpp>
#include "DrawableMixin.hpp"
#include "Spline.hpp"

constexpr float kEpsVertical = 1e-8f;

enum class EdgeType {CatmullRom, Linear, Circular, Helix};

struct NodeMeta
{
    bool stationStart = false;
    bool stationEnd = false;
    float stationLength = 0.f;
    float userRoll = 0.f;
};

struct EdgeMeta
{
    EdgeType type {EdgeType::CatmullRom};
};

struct RollKey
{
    float s, roll;
};

struct Frame
{
    glm::vec3 pos;
    glm::vec3 T; //styczna
    glm::vec3 N; //wektor normalny ramki "do góry"
    glm::vec3 B; //binormalny "w prawo"
    float s; // droga od początku trasy
};

struct Sample
{
    glm::vec3 pos, tan;
};

class PathSampler
{
public:
    PathSampler(const Spline& spline, const std::vector<EdgeMeta>& e) : spline_(spline), edge_(e) {}

    Sample sampleAtS(float s) const;
private:
    const Spline& spline_;
    const std::vector<EdgeMeta>& edge_;
};

class Track : public DrawableMixin<Track>{

public:
    Track();

    std::vector<Frame> buildPTF (const Spline& spline, float ds, glm::vec3 globalUp) const;

    static float approximateSForPoint(const Spline& spline, const glm::vec3& p, float s0, float s1, float ds);

    float stationEdgeFadeWeight(float s) const;
    bool isInStation(float s) const;
    bool isNearStationEdge(float s) const;
    void buildStationIntervals(const Spline& spline, float sampleStep);

    void rebuildRollKeys(const Spline& spline, float sampleStep);
    float manualRollAtS(const Spline& spline, float s) const;

    void syncMetaWithSpline(const Spline& spline);
    void rebuildMeta(const Spline& spline);

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
