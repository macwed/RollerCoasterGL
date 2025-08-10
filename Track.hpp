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
    std::vector<Frame> buildPTF (float ds, glm::vec3 globalUp, Spline& spline);

    void uploadToGPU();
    void draw() const;

private:
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
