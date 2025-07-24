//
// Created by maciej on 15.07.25.
//

#ifndef SIMPLEX_NOISE_HPP
#define SIMPLEX_NOISE_HPP
#include <vector>
#include "glm/glm.hpp"

class SimplexNoise {
public:
    explicit SimplexNoise(unsigned seed);

    // core API
    [[nodiscard]] float noise(float x, float y) const;

    [[nodiscard]] float fbm(float x, float y, float frequency = 1.5f, int octaves = 8, float lacunarity = 2.0f, float persistence = 0.5f) const;

private:
    unsigned seed_;
    std::vector<int> perm_;
    std::vector<glm::vec2> gradient_table_;

    void init_perm_();
    void init_gradient_table_();
};

#endif //SIMPLEX_NOISE_HPP
