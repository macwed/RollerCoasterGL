//
// Created by maciej on 20.09.25.
//

#ifndef MESH_HPP
#define MESH_HPP

#include <vector>
#include <glad.h>
#include <glm/glm.hpp>
#include "gfx/geometry/RailGeometryBuilder.hpp" // dla geometry::Vertex

namespace rc::gfx::render {

class Mesh {
public:
    ~Mesh(){ release(); }

    void setData(const std::vector<geometry::Vertex>& verts,
                 const std::vector<uint32_t>& indices) {
        cpuVerts_ = verts;
        cpuIdx_   = indices;
        indexCount_ = cpuIdx_.size();
        uploaded_ = false;
    }

    void uploadToGPU() {
        if (uploaded_) return;
        release();

        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(cpuVerts_.size() * sizeof(geometry::Vertex)),
                     cpuVerts_.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(cpuIdx_.size() * sizeof(uint32_t)),
                     cpuIdx_.data(), GL_STATIC_DRAW);

        // pozycja(0), normalna(1), UV(2)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(geometry::Vertex),
                              reinterpret_cast<void*>(offsetof(geometry::Vertex, pos)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(geometry::Vertex),
                              reinterpret_cast<void*>(offsetof(geometry::Vertex,normal)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(geometry::Vertex),
                              reinterpret_cast<void*>(offsetof(geometry::Vertex,uv)));

        glBindVertexArray(0);
        uploaded_ = true;
    }

    void draw() const {
        if (!uploaded_) return;
        glBindVertexArray(vao_);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount_), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    void release() {
        if (ebo_) glDeleteBuffers(1,&ebo_);
        if (vbo_) glDeleteBuffers(1,&vbo_);
        if (vao_) glDeleteVertexArrays(1,&vao_);
        vao_=vbo_=ebo_=0;
        uploaded_ = false;
    }

private:
    GLuint vao_=0, vbo_=0, ebo_=0;
    size_t indexCount_=0;
    bool uploaded_=false;
    std::vector<geometry::Vertex> cpuVerts_;
    std::vector<uint32_t> cpuIdx_;
};
} // namespace rc::gfx::render

#endif //MESH_HPP
