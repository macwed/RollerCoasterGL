//
// Created by maciej on 21.07.25.
//

#include "Terrain.hpp"

#include <algorithm>
#include <cmath>
#include <glad.h>
#include <glm/glm.hpp>
#include <limits>
#include <vector>

#include "SimplexNoise.hpp"
#include "math/Array_2D.hpp"

Terrain::Terrain(int width, int height, int seed) :
    width_(width), height_(height), heightmap_(width, height), noise_(seed), vbo_(0), vao_(0), ibo_(0) {}
void Terrain::releaseGL() {
    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (ibo_) {
        glDeleteBuffers(1, &ibo_);
        ibo_ = 0;
    }
    if (vao_) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
}


void Terrain::generate(float scale, float frequency, int octaves, float lacunarity, float persistence, float exponent,
                       float height_scale) {

    for (int y = 0; y < height_; y++) {
        for (int x = 0; x < width_; x++) {
            float h = noise_.fbm(static_cast<float>(x) * scale + offset_, static_cast<float>(y) * scale + offset_,
                                 frequency, octaves, lacunarity, persistence);
            heightmap_(x, y) = h;
        }
    }
    heightmap_.normalize();

    vertices_.clear();
    indices_.clear();
    vertices_.reserve(width_ * height_);
    indices_.reserve((width_ - 1) * (height_ - 1) * 6);

    for (int y = 0; y < height_; y++) {
        for (int x = 0; x < width_; x++) {
            vertices_.emplace_back(x, std::pow(heightmap_(x, y), exponent) * height_scale, y);
        }
    }
    for (int y = 0; y < height_ - 1; y++) {
        for (int x = 0; x < width_ - 1; x++) {
            unsigned int x0y0 = y * width_ + x;
            unsigned int x0y1 = (y + 1) * width_ + x;
            unsigned int x1y0 = y * width_ + (x + 1);
            unsigned int x1y1 = (y + 1) * width_ + (x + 1);

            // triangulacja siatki
            indices_.emplace_back(x0y0);
            indices_.emplace_back(x0y1);
            indices_.emplace_back(x1y0);

            indices_.emplace_back(x0y1);
            indices_.emplace_back(x1y1);
            indices_.emplace_back(x1y0);
        }
    }

    normals_.resize(vertices_.size(), glm::vec3(0.0f));

    // oblicz face normals dla trójkątów
    for (size_t i = 0; i < indices_.size(); i += 3) {
        const unsigned int ia = indices_[i];
        const unsigned int ib = indices_[i + 1];
        const unsigned int ic = indices_[i + 2];

        const glm::vec3& A = vertices_[ia];
        const glm::vec3& B = vertices_[ib];
        const glm::vec3& C = vertices_[ic];

        glm::vec3 U = B - A;
        glm::vec3 V = C - A;
        glm::vec3 N = glm::normalize(glm::cross(U, V));

        normals_[ia] += N;
        normals_[ib] += N;
        normals_[ic] += N;
    }
    for (auto& n: normals_)
        n = glm::normalize(n);

    float mPerTile = 2.f;

    gpuVerts_.resize(vertices_.size());
    for (size_t y = 0; y < height_; y++) {
        for (size_t x = 0; x < width_; x++) {
            size_t idx = y * width_ + x;
            glm::vec2 uv = glm::vec2(x, y) / mPerTile;
            gpuVerts_[idx] = {vertices_[idx], normals_[idx], uv};
        }
    }
}

float Terrain::sampleHeightBilinear(float x, float z) const {
    int ix = static_cast<int>(std::floor(x));
    int iz = static_cast<int>(std::floor(z));
    float fx = x - static_cast<float>(ix);
    float fz = z - static_cast<float>(iz);

    auto H = [&](int X, int Z) {
        X = std::clamp(X, 0, width_  - 1);
        Z = std::clamp(Z, 0, height_ - 1);
        return vertices_[Z * width_ + X].y;
    };

    float h00 = H(ix,   iz  );
    float h10 = H(ix+1, iz  );
    float h01 = H(ix,   iz+1);
    float h11 = H(ix+1, iz+1);

    float hx0 = glm::mix(h00, h10, fx);
    float hx1 = glm::mix(h01, h11, fx);
    return glm::mix(hx0, hx1, fz);
}

void Terrain::uploadToGPU() {

    if (vao_)
        glDeleteVertexArrays(1, &vao_);
    if (vbo_)
        glDeleteBuffers(1, &vbo_);
    if (ibo_)
        glDeleteBuffers(1, &ibo_);

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ibo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(gpuVerts_.size() * sizeof(TVertex)), gpuVerts_.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TVertex), reinterpret_cast<void*>(offsetof(TVertex, pos)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TVertex), reinterpret_cast<void*>((offsetof(TVertex, nrm))));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TVertex), reinterpret_cast<void*>((offsetof(TVertex, uv))));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices_.size() * sizeof(unsigned int)),
                 indices_.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void Terrain::draw() const {

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

float Terrain::minH() const {
    float minHeight = std::numeric_limits<float>::max();
    for (auto& v: vertices_) {
        minHeight = std::min(minHeight, v.y);
    }
    return minHeight;
}
float Terrain::maxH() const {
    float maxHeight = std::numeric_limits<float>::lowest();
    for (auto& v: vertices_) {

        maxHeight = std::max(maxHeight, v.y);
    }
    return maxHeight;
}
