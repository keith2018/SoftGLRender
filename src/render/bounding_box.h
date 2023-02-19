/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "base/glm_inc.h"

namespace SoftGL {

class BoundingBox {
 public:
  BoundingBox() = default;
  BoundingBox(glm::vec3 &a, glm::vec3 &b) : min(a), max(b) {}

  void GetCorners(glm::vec3 *dst) const;
  BoundingBox Transform(const glm::mat4 &matrix) const;
  bool Intersects(const BoundingBox &box) const;
  void Merge(const BoundingBox &box);

 public:
  glm::vec3 min{0.f, 0.f, 0.f};
  glm::vec3 max{0.f, 0.f, 0.f};
};

}
