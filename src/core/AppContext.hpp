//
// Created by maciej on 30.07.25.
//

#ifndef APPCONTEXT_HPP
#define APPCONTEXT_HPP

#include "../camera/FreeFlyCam.hpp"
#include "../terrain/Terrain.hpp"

enum class CamMode { Free, Ride, Chase };

struct ProjectConfig {
    int windowWidth, windowHeight;

    glm::vec3 camPos;
    int mapWidth, mapHeight;
    int noiseSeed;

    float noiseScale;
    float noiseFreq;
    int noiseOctaves;
    float noiseLacunarity;
    float noisePersistence;
    float noiseExponent;
    float noiseHeightScale;
};

struct AppContext {

    FreeFlyCam camera;
    Terrain terrain;
    bool keys[1024]{false};
    float lastFrame = 0.0f, currentFrame = 0.0f, deltaTime = 0.0f;
    int polygonMode = GL_FILL;
    bool cursorLocked = true;

    bool showTerrainPanel = false;

    CamMode camMode = CamMode::Free;
    glm::vec3 smoothedEye{0};
    glm::vec3 smoothedTarget{0};
    bool camInitialized = false;

    explicit AppContext(const ProjectConfig& cfg) :
        camera(cfg.camPos), terrain(cfg.mapWidth, cfg.mapHeight, cfg.noiseSeed) {}
};


#endif // APPCONTEXT_HPP
