//
// Created by maciej on 20.09.25.
//

#include "SupportGeometryBuilder.hpp"


float beamSpacing = 5.f;


namespace rc::gfx::geometry {

    void SupportGeometryBuilder::addOrientedBox(const glm::vec3& C,
                                            const glm::vec3& axisDir,
                                            float L, float sx, float sy)
{
    glm::vec3 T = glm::normalize(axisDir);// oś dłuższa
    glm::vec3 up(0,1,0);
    // baza poprzeczki
    glm::vec3 X = glm::normalize((std::abs(glm::dot(T,up)) < 0.99f)
        ? glm::cross(up, T)
        : glm::cross(glm::vec3(1,0,0), T));
    glm::vec3 Y = glm::normalize(glm::cross(T, X));

    glm::vec3 hx = X * (sx * 0.5f);
    glm::vec3 hy = Y * (sy * 0.5f);
    glm::vec3 hL = T * (L  * 0.5f);

    // 8 rogów
    glm::vec3 p[8] = {
        C - hL - hx - hy, C - hL + hx - hy, C - hL + hx + hy, C - hL - hx + hy,
        C + hL - hx - hy, C + hL + hx - hy, C + hL + hx + hy, C + hL - hx + hy
    };

    // 24 wierzchołki (z normalnymi per-face) + indeksy 12 trójkątów

    //auto baseIndex = static_cast<uint32_t>(mesh_.vertices.size());
        auto pushQuad = [&](int a,int b,int c,int d, const glm::vec3& n){
            uint32_t i0 = static_cast<uint32_t>(mesh_.vertices.size());
            mesh_.vertices.push_back({p[a], n, {0,0}});

            uint32_t i1 = static_cast<uint32_t>(mesh_.vertices.size());
            mesh_.vertices.push_back({p[b], n, {1,0}});

            uint32_t i2 = static_cast<uint32_t>(mesh_.vertices.size());
            mesh_.vertices.push_back({p[c], n, {1,1}});

            uint32_t i3 = static_cast<uint32_t>(mesh_.vertices.size());
            mesh_.vertices.push_back({p[d], n, {0,1}});

            mesh_.indices.insert(mesh_.indices.end(), { i0, i1, i2,  i0, i2, i3 });
        };

    // 6 ścian (normalne przybliżone)
    pushQuad(0,1,2,3, -T); // tył
    pushQuad(4,5,6,7,  T); // przód
    pushQuad(0,4,7,3, -X); // -X
    pushQuad(1,5,6,2,  X); // +X
    pushQuad(0,1,5,4, -Y); // -Y
    pushQuad(3,2,6,7,  Y); // +Y
}

    void SupportGeometryBuilder::addBeamBox(const glm::vec3& left,
                                        const glm::vec3& right,
                                        float thickness, float height)
    {
        glm::vec3 mid = 0.5f*(left+right);
        glm::vec3 dir = right - left;
        float L = glm::length(dir);
        if (L < 1e-6f) return;
        addOrientedBox(mid, dir/L, L, /*sx=*/thickness, /*sy=*/height);
    }

    void SupportGeometryBuilder::addOrientedCylinder(const glm::vec3& A,
                                                 const glm::vec3& B,
                                                 float r, int sides)
    {
        glm::vec3 axis = B - A;
        float L = glm::length(axis);
        if (L < 1e-5f) return;
        glm::vec3 T = axis / L;

        // baza poprzeczki
        glm::vec3 up(0,1,0);
        glm::vec3 X = glm::normalize((std::abs(glm::dot(T,up)) < 0.99f)
            ? glm::cross(up, T)
            : glm::cross(glm::vec3(1,0,0), T));
        glm::vec3 Y = glm::normalize(glm::cross(T, X));

        uint32_t base = static_cast<uint32_t>(mesh_.vertices.size());

        // dwa pierścienie: top i bottom
        for (int ring = 0; ring < 2; ++ring) {
            glm::vec3 C = (ring == 0) ? A : B;
            for (int i = 0; i <= sides; ++i) {
                float u = static_cast<float>(i) / static_cast<float>(sides);
                float ang = u * twoPi;
                glm::vec3 n = std::cos(ang)*X + std::sin(ang)*Y;       // normal boków
                glm::vec3 p = C + r*n;
                mesh_.vertices.push_back({ p, n, {u, ring ? 1.f : 0.f} });
            }
        }

        // boki
        uint32_t ringStride = sides + 1;
        for (int i = 0; i < sides; ++i) {
            uint32_t a = base + i;
            uint32_t b = base + i + 1;
            uint32_t c = base + ringStride + i;
            uint32_t d = base + ringStride + i + 1;
            mesh_.indices.insert(mesh_.indices.end(), { a,b,c, b,d,c });
        }
    }

    void SupportGeometryBuilder::addSupportCylinder(const glm::vec3& top, const glm::vec3& bottom, float r, int sides) {
        addOrientedCylinder(top, bottom, r, sides);
    }

    void SupportGeometryBuilder::addFootDisk(const glm::vec3& C, float R, int sides) {
        uint32_t centerIdx = static_cast<uint32_t>(mesh_.vertices.size());
        mesh_.vertices.push_back({ C, {0,1,0}, {0.5f,0.5f} });
        for (int i = 0; i <= sides; ++i) {
            float u = static_cast<float>(i) / static_cast<float>(sides);
            float ang = u * twoPi;
            glm::vec3 p = C + glm::vec3(std::cos(ang)*R, 0, std::sin(ang)*R);
            mesh_.vertices.push_back({ p, {0,1,0}, {0.5f+0.5f*std::cos(ang), 0.5f+0.5f*std::sin(ang)} });
        }
        for (int i = 0; i < sides; ++i) {
            mesh_.indices.insert(mesh_.indices.end(), {
                centerIdx, centerIdx+1+i, centerIdx+1+i+1
            });
        }
    }

} // rc::gfx::geometry