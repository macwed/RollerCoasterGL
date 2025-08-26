//
// Created by maciej on 21.07.25.
//

#ifndef TERRAIN_HPP
#define TERRAIN_HPP

#include <glm/glm.hpp>
#include <vector>

#include "SimplexNoise.hpp"
#include "math/Array_2D.hpp"


class Terrain {
public:
    Terrain(int width, int height, int seed);
    void releaseGL();

    void generate(float scale, float frequency = 0.01f, int octaves = 8, float lacunarity = 2.0f,
                  float persistence = 0.5f, float exponent = 1.0f, float height_scale = 20.0f);
    void uploadToGPU();
    void draw() const;

    [[nodiscard]] float getHeight(int x, int y) const;

    [[nodiscard]] float minH() const;
    [[nodiscard]] float maxH() const;

private:
    static constexpr float offset_ = 137.0f;
    int width_, height_;
    Array_2D<float> heightmap_;
    std::vector<glm::vec3> vertices_;
    std::vector<unsigned int> indices_;
    std::vector<glm::vec3> normals_;
    SimplexNoise noise_;
    unsigned vbo_, vao_, ibo_;
};


#endif // TERRAIN_HPP
