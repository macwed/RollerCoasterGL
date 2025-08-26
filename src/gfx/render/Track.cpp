//
// Created by maciej on 02.08.25.
//

#include "Track.hpp"

#include <glad.h>
#include <glm/glm.hpp>
#include <span>

namespace rc::gfx::render {
    /*-------------------------------------------------TRACK::TRACK-------------------------------------------------------*/
    Track::Track() : vbo_(0), vao_(0), ibo_(0) {}
    Track::~Track() {
        releaseGL();
    }

    /*-------------------------------------------------TRACK::OpenGL&GPU--------------------------------------------------*/

    void Track::releaseGL() {
        if (vao_) {
            glDeleteVertexArrays(1, &vao_);
            vao_ = 0;
        }
        if (vbo_) {
            glDeleteBuffers(1, &vbo_);
            vbo_ = 0;
        }
        if (ibo_) {
            glDeleteBuffers(1, &ibo_);
            ibo_ = 0;
        }
    }

    void Track::setMesh(std::span<const glm::vec3> vertices, std::span<const uint32_t> indices) {
        points_.assign(vertices.begin(), vertices.end());
        indices_.assign(indices.begin(), indices.end());
    }

    void Track::uploadToGPU() {
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
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(points_.size() * sizeof(glm::vec3)), points_.data(),
                     GL_STATIC_DRAW);

        // Position-only layout: tightly packed glm::vec3
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
        glEnableVertexAttribArray(0);


        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices_.size() * sizeof(glm::uint32_t)),
                     indices_.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    void Track::draw() const {
        glBindVertexArray(vao_);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }
} // namespace rc::gfx::render
