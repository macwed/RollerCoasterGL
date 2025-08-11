//
// Created by maciej on 02.08.25.
//

#ifndef TRACK_HPP
#define TRACK_HPP
#include <vector>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include "DrawableMixin.hpp"
#include "Spline.hpp"

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
    float s;
    float roll;
};

struct Frame
{
    glm::vec3 pos;
    glm::vec3 T; //styczna
    glm::vec3 N; //wektor normalny ramki "do góry"
    glm::vec3 B; //binormalny "w prawo"
    float s; // droga od początku trasy
};

class Track : public DrawableMixin<Track>{

public:
    Track();
    void releaseGL();

    void pushPoint(float x, float y, float z);
    void popPoint();
    std::vector<Frame> buildPTF (const Spline& spline, float ds, glm::vec3 globalUp, float l_station,
                                   std::function<float(float)> rollAtS);

    static float approximateSForPoint(const Spline& spline, const glm::vec3& p, float s0, float s1, float ds);
    void buildStationIntervals(const Spline& spline, float sampleStep);

    void uploadToGPU();
    void draw() const;

private:
    std::vector<NodeMeta> nodeMeta_;
    std::vector<EdgeMeta> edgeMeta_;
    std::vector<std::pair<float, float>> stationIntervals_;
    std::vector<RollKey> rollKeys_;
    std::vector<glm::vec3> points_;
    std::vector<glm::uint32_t> indices_;
    unsigned vbo_, vao_, ibo_;

    static inline glm::vec3 rotateAroundAxis(const glm::vec3& v, const glm::vec3& axis, float angle)
    {
        glm::quat q = glm::angleAxis(angle, axis);
        return q * v;
    }

    /*glm::vec3 interpolate()
    {
        //TODO
    }*/
};



#endif //TRACK_HPP
