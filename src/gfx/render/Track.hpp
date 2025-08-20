//
// Created by maciej on 02.08.25.
//

#ifndef TRACK_HPP
#define TRACK_HPP
#include <vector>
#include <glm/gtc/quaternion.hpp>
#include "gfx/render/DrawableMixin.hpp"

namespace rc::gfx::render {
constexpr float kEpsVertical = 1e-8f;

class Track : public DrawableMixin<Track> {
public:
  Track();

  void uploadToGPU();
  void draw() const;
  void releaseGL();

private:

  std::vector<glm::vec3> points_;
  std::vector<std::uint32_t> indices_;
  unsigned vbo_, vao_, ibo_;

  const float feather = 0.75f;
};
}



#endif //TRACK_HPP
