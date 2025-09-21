//
// Created by maciej on 21.07.25.
//

#ifndef TERRAIN_HPP
#define TERRAIN_HPP

#include <glm/glm.hpp>
#include <vector>

#include "SimplexNoise.hpp"
#include "glad.h"
#include "math/Array_2D.hpp"

struct TVertex {
    glm::vec3 pos;
    glm::vec3 nrm;
    glm::vec2 uv;
};

class Terrain {
public:
    Terrain(int width, int height, int seed);
    void releaseGL();

    void generate(float scale, float frequency = 0.01f, int octaves = 8, float lacunarity = 2.0f,
                  float persistence = 0.5f, float exponent = 1.0f, float height_scale = 20.0f);
    [[nodiscard]] float sampleHeightBilinear(float x, float z) const;
    void uploadToGPU();
    void draw() const;

    [[nodiscard]] float minH() const;
    [[nodiscard]] float maxH() const;

    float worldHeightAt(int x, int z) const {
        x = std::clamp(x, 0, width_  - 1);
        z = std::clamp(z, 0, height_ - 1);
        return vertices_[z * width_ + x].y;
    }

private:
    static constexpr float offset_ = 137.0f;
    int width_, height_;
    Array_2D<float> heightmap_;
    std::vector<glm::vec3> vertices_;
    std::vector<unsigned int> indices_;
    std::vector<glm::vec3> normals_;
    std::vector<TVertex> gpuVerts_;
    SimplexNoise noise_;
    GLuint vbo_, vao_, ibo_;
};


#endif // TERRAIN_HPP
