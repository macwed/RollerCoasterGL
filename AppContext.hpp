//
// Created by maciej on 30.07.25.
//

#ifndef APPCONTEXT_HPP
#define APPCONTEXT_HPP
#include "FreeFlyCam.hpp"
#include "Terrain.hpp"


struct AppContext {

    FreeFlyCam camera;
    Terrain terrain;
    bool keys[1024]{false};
    float lastFrame;
    float currentFrame;
    float deltaTime;
    AppContext() :
        camera(glm::vec3(50.0f, 50.0f, 150.0f)),
        terrain(512, 512, 4567890),
        lastFrame(0.0f),
        currentFrame(0.0f),
        deltaTime(0.0f)
    {}

};



#endif //APPCONTEXT_HPP
