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
    init_gradient_table();
}

float SimplexNoise::noise(float x, float y) const {
    //skewing
    float s = (x + y) * F2;
    int i = static_cast<int>(std::floor(x + s));
    int j = static_cast<int>(std::floor(y + s));

    //unskewing
    float t = (i + j) * G2;
    float x0 = x - (i - t);
    float y0 = y - (j - t);

    int i1, j1;
    if (x0 > y0) {
        i1 = 1; j1 = 0;
    } else {
        i1 = 0; j1 = 1;
    }

    unsigned gi0 = perm_[(perm_[((i % 256) + 256) % 256] + ((j % 256) + 256)) % 256] % 32;
    unsigned gi1 = perm_[(perm_[((i + i1) % 256 + 256) % 256] + ((j + j1) % 256 + 256)) % 256] % 32;
    unsigned gi2 = perm_[(perm_[((i + 1) % 256 + 256) % 256] + ((j + 1) % 256 + 256)) % 256] % 32;

    float x1 = x0 - i1;
    float y1 = y0 - j1;
    float x2 = x0 - 1.0f;
    float y2 = y0 - 1.0f;

    float dot0 = glm::dot(gradient_table_[gi0], glm::vec2(x0, y0));
    float dot1 = glm::dot(gradient_table_[gi1], glm::vec2(x1, y1));
    float dot2 = glm::dot(gradient_table_[gi2], glm::vec2(x2, y2));

    //dot products weights
    float t0 = 0.5f - x0*x0 - y0*y0;
    float t1 = 0.5f - x1*x1 - y1*y1;
    float t2 = 0.5f - x2*x2 - y2*y2;

    float contrib0 = 0.0f, contrib1 = 0.0f, contrib2 = 0.0f;
    if (t0 > 0) contrib0 = pow(t0, 4) * dot0;
    if (t1 > 0) contrib1 = pow(t1, 4) * dot1;
    if (t2 > 0) contrib2 = pow(t2, 4) * dot2;

    float noise = 70.0f * (contrib0 + contrib1 + contrib2);

    return noise;
}

void SimplexNoise::init_perm_() {
    std::mt19937 gen(seed_);
    perm_.resize(256);
    for (int i = 0; i < 256; i++) {
        perm_[i] = i;
    }
    std::ranges::shuffle(perm_, gen);
}


void SimplexNoise::init_gradient_table() {
    gradient_table_.clear();
    for (int i = 0; i < 32; i++) {
        float angle = i * 2.0f * static_cast<float>(M_PI) / 32.0f;
        gradient_table_.emplace_back(std::cos(angle), std::sin(angle));
    }
}
