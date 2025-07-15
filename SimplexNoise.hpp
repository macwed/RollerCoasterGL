//
// Created by maciej on 15.07.25.
//

#ifndef SIMPLEX_NOISE_HPP
#define SIMPLEX_NOISE_HPP
#include <vector>

class SimplexNoise {
public:
    SimplexNoise(unsigned seed, unsigned width, unsigned height)
        : seed_(seed), width_(width), height_(height),
          grid_(height, std::vector<float>(width)),
          gradient_table_(height, std::vector<float>(width))
    {}

    // core API
    float noise(float x, float y) const;

private:
    unsigned seed_;
    unsigned width_;
    unsigned height_;
    std::vector<std::vector<float>> grid_;
    std::vector<std::vector<float>> gradient_table_;
};

#endif //SIMPLEX_NOISE_HPP
