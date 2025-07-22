//
// Created by maciej on 21.07.25.
//

#include <vector>
#include <glm/glm.hpp>
#include "Array_2D.hpp"
#include "SimplexNoise.hpp"
#include "Terrain.hpp"



Terrain::Terrain(int width, int height, unsigned seed)
    : width_(width), height_(height), noise_(seed), heightmap_(width, height) {}

Terrain::generate(float frequency, int octaves, float lacunarity, float persistence, float exponent);
Terrain::uploadToGPU();
Terrain::draw();

Terrain::getHeight(int x, int y) const;