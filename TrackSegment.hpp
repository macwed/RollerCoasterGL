//
// Created by maciej on 07.08.25.
//

#ifndef TRACKSEGMENT_HPP
#define TRACKSEGMENT_HPP
#include <vector>
#include <glm/vec3.hpp>


class TrackSegment {
    glm::vec3 startPoint_, endPoint_;
    glm::vec3 direction_;
    float length_;
    float width_;
    virtual std::vector<glm::vec3> build_();
public:
    virtual float length() const = 0;
    virtual glm::vec3 getPointAt(float t) const = 0;
    virtual ~TrackSegment() = default;

};

class StraightSegment : public TrackSegment
{

};

class TurnSegment : public TrackSegment
{

};



#endif //TRACKSEGMENT_HPP
