//
// Created by maciej on 30.07.25.
//

#ifndef APPCONTEXT_HPP
#define APPCONTEXT_HPP
#include "FreeFlyCam.hpp"
#include "Terrain.hpp"


struct ProjectConfig
{
    int windowWidth, windowHeight;

    glm::vec3 camPos;
    int mapWidth, mapHeight;
    unsigned noiseSeed;

    float noiseScale;
    float noiseFreq;
    float noiseOctaves;
    float noiseLacunarity;
    float noisePersistence;
    float noiseExponent;
    float noiseHeightScale;

};

struct AppContext
{

    FreeFlyCam camera;
    Terrain terrain;
    bool keys[1024]{false};
    float lastFrame;
    float currentFrame;
    float deltaTime;

    explicit AppContext(const ProjectConfig& cfg) :
        camera(cfg.camPos),
        terrain(cfg.mapWidth, cfg.mapHeight, cfg.noiseSeed),
        lastFrame(0.0f),
        currentFrame(0.0f),
        deltaTime(0.0f)
    {}

};



#endif //APPCONTEXT_HPP
