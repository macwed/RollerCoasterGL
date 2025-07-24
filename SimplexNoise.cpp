//
// Created by maciej on 17.07.25.
//

#include "SimplexNoise.hpp"
#include "glm/glm.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

constexpr float F2 = 0.36602540378f; // (sqrt(3)-1)/2
constexpr float G2 = 0.2113248654f;  // (3-sqrt(3))/6

SimplexNoise::SimplexNoise(unsigned seed) : seed_(seed) {
    init_perm_();
    init_gradient_table_();
}

float SimplexNoise::noise(float x, float y) const
{
    //skewing
    float s = (x + y) * F2;
    int i = static_cast<int>(std::floor(x + s));
    int j = static_cast<int>(std::floor(y + s));

    //unskewing
    float t = static_cast<float>((i + j)) * G2;
    float x0 = x - (static_cast<float>(i) - t);
    float y0 = y - (static_cast<float>(j) - t);

    int i1, j1;
    if (x0 > y0) {
        i1 = 1; j1 = 0;
    } else {
        i1 = 0; j1 = 1;
    }

    int ii = i & 255;
    int jj = j & 255;

    unsigned gi0 = perm_[(ii) + perm_[(jj)]] % 16;
    unsigned gi1 = perm_[(ii + i1) + perm_[(jj + j1)]] % 16;
    unsigned gi2 = perm_[(ii + 1) + perm_[(jj + 1)]] % 16;

    float x1 = x0 - static_cast<float>(i1) + G2;
    float y1 = y0 - static_cast<float>(j1) + G2;
    float x2 = x0 - 1.0f + 2 * G2;
    float y2 = y0 - 1.0f + 2 * G2;

    float dot0 = glm::dot(gradient_table_[gi0], glm::vec2(x0, y0));
    float dot1 = glm::dot(gradient_table_[gi1], glm::vec2(x1, y1));
    float dot2 = glm::dot(gradient_table_[gi2], glm::vec2(x2, y2));

    //dot products weights
    float t0 = 0.5f - x0*x0 - y0*y0;
    float t1 = 0.5f - x1*x1 - y1*y1;
    float t2 = 0.5f - x2*x2 - y2*y2;

    float contrib0 = 0.0f, contrib1 = 0.0f, contrib2 = 0.0f;
    if (t0 > 0) contrib0 = static_cast<float>(pow(t0, 4)) * dot0;
    if (t1 > 0) contrib1 = static_cast<float>(pow(t1, 4)) * dot1;
    if (t2 > 0) contrib2 = static_cast<float>(pow(t2, 4)) * dot2;

    float noise = 70.0f * (contrib0 + contrib1 + contrib2);

    return noise;
}

float SimplexNoise::fbm(float x, float y, float frequency, int octaves, float lacunarity, float persistence) const {

    float total = 0.0f;
    float amplitude = 1.0f;

    for (int i = 0; i < octaves; i++) {
        total += noise(x * frequency, y * frequency) * amplitude;
        frequency *= lacunarity;
        amplitude *= persistence;
    }

    return total;
}

void SimplexNoise::init_perm_() {
    std::mt19937 gen(seed_);
    perm_.resize(256 * 2);
    for (int i = 0; i < 256; i++) {
        perm_[i] = i;
    }
    std::shuffle(perm_.begin(), perm_.begin() + 256, gen);

    for (int i = 0; i < 256; i++)
    {
        perm_[256 + i] = perm_[i];
    }
}


void SimplexNoise::init_gradient_table_() {
    gradient_table_.clear();
    for (int i = 0; i < 16; i++) {
        float angle = static_cast<float>(i) * 2.0f * static_cast<float>(M_PI) / 16.0f;
        gradient_table_.emplace_back(std::cos(angle), std::sin(angle));
    }
}
