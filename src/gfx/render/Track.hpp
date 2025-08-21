//
// Created by maciej on 02.08.25.
//

#ifndef TRACK_HPP
#define TRACK_HPP
#include <glm/vec3.hpp>
#include <span>
#include <vector>

#include "gfx/render/DrawableMixin.hpp"

namespace rc::gfx::render {
class Track : public DrawableMixin<Track> {
public:
  Track();
  ~Track();

  void setMesh(std::span<const glm::vec3> vertices, std::span<const uint32_t> indices);
  void uploadToGPU();
  void draw() const;
  void releaseGL();

private:

  std::vector<glm::vec3> points_;
  std::vector<std::uint32_t> indices_;
  unsigned vbo_, vao_, ibo_;

  // no extra state yet; normals/uv to be added with future shaders
};
}



#endif //TRACK_HPP
