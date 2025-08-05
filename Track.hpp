//
// Created by maciej on 02.08.25.
//

#ifndef TRACK_HPP
#define TRACK_HPP
#include <vector>
#include <glm/vec3.hpp>
#include "DrawableMixin.hpp"


class Track : public DrawableMixin<Track>{

public:
    Track();
    void releaseGL();

    void pushPoint(float x, float y, float z);
    void popPoint();

    void uploadToGPU();
    void draw() const;

private:
    std::vector<glm::vec3> points_;
    unsigned vbo_, vao_, ibo_;

    /*glm::vec3 interpolate()
    {
        //TODO
    }*/
};



#endif //TRACK_HPP
