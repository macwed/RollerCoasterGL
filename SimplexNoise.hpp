//
// Created by maciej on 15.07.25.
//

#ifndef SIMPLEX_NOISE_HPP
#define SIMPLEX_NOISE_HPP
#include <vector>
#include "glm/glm.hpp"

class SimplexNoise {
public:
    SimplexNoise(unsigned seed);

    // core API
    float noise(float x, float y) const;

    float fbm(float x, float y, int octaves, float persistence) const;

private:
    unsigned seed_;
    std::vector<int> perm_;
    std::vector<glm::vec2> gradient_table_;

    void init_perm_();
    void init_gradient_table_();
};

#endif //SIMPLEX_NOISE_HPP
