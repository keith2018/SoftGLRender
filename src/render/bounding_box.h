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
  BoundingBox(const glm::vec3 &a, const glm::vec3 &b) : min(a), max(b) {}

  void getCorners(glm::vec3 *dst) const;
  BoundingBox transform(const glm::mat4 &matrix) const;
  bool intersects(const BoundingBox &box) const;
  void merge(const BoundingBox &box);

 protected:
  static void updateMinMax(glm::vec3 *point, glm::vec3 *min, glm::vec3 *max);

 public:
  glm::vec3 min{0.f, 0.f, 0.f};
  glm::vec3 max{0.f, 0.f, 0.f};
};

}
